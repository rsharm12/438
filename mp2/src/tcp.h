#ifndef TCP_H
#define TCP_H


#include <thread>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <chrono>
#include <ctime>
#include <atomic>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <map>
#include <algorithm>

/* networking header files */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>

#define PACKETSIZE  1472
#define HEADERSIZE  12
#define DATASIZE    (PACKETSIZE - HEADERSIZE)
/*
 * HEADER:
 *        4 bytes - seq number
 *        4 bytes - ack number
 *        2 bytes - flags
 *        2 bytes - rcv window
 * DATA:
 *        up to 1460 bytes - data
 */

/* TCP FLAG BITMAP */
#define FLAG_ACK   0x1
#define FLAG_FIN   0x2

using namespace std;

class TCP {

public:

    struct Header {
        unsigned int seqNum;
        unsigned int ackNum;
        unsigned short flags;
        unsigned short rcwnd;
        Header() {
            seqNum = 0;
            ackNum = 0;
            flags  = 0;
            rcwnd  = 0;
        }
        void log() {
            cout << " SEQ=" << seqNum;
            cout << " ACK=" << ackNum;
            cout << " FL="  << flags;
            cout << " RW="  << rcwnd << endl;
        }
    };

    struct Packet {
        unsigned long long int pkt_length;
        char packet[DATASIZE];

        Packet() {
            pkt_length = 0;
        }
    };

    struct TCP_cgst_ctrl {
        unsigned int cwnd;
        unsigned int ssthresh;
        unsigned int dupACKcount;

        TCP_cgst_ctrl()
        {
            cwnd = 1460;
            ssthresh = 64000;
            dupACKcount = 0;
        }

        void log() {
            cout << " cwnd=" << cwnd;
            cout << " ssthresh=" << ssthresh;
            cout << " dupACK="  << dupACKcount << endl;
        } 
    };

    TCP(unsigned int port);
    ~TCP();

    static void senderReceiveACK(TCP *tcp);
    static void senderSendData(TCP *tcp, FILE* fp,
                               unsigned long long int bytesToTransfer);
    static void receiverReceiveData(TCP * tcp, FILE* fp);
    static void receiverSendACK(TCP * tcp);

    void setupSockaddrOther(const char * hostname);
    void senderCloseConnection();

    int sock;
    unsigned int udpPort;
    struct sockaddr_in si_me;
    struct sockaddr_in si_other;
    socklen_t slen = (socklen_t) sizeof(struct sockaddr_in);

    /* timing */
    chrono::high_resolution_clock::time_point starttime;
    chrono::high_resolution_clock::time_point endtime;

    /* receiver local variables */
    mutex rcvrACK_mtx;
    queue<unsigned int> rcvrACK_q;

    /* sender local variables */
    TCP_cgst_ctrl sender_ctrl;
    unordered_map<unsigned int, Packet*> packets_sent; /* seqNum -> packet */
    unsigned int sendBase;

    static Header extractHeader(const char *packet);

private:

    void diep(const char *s) {
        perror(s);
        exit(1);
    }

    void setupSockaddrMe(unsigned int udpPort)
    {
        /* set up me */
        memset((void *) &si_me, 0, sizeof (si_me));
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(udpPort);
        si_me.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    int createAndSendPacket(const char * data, int dataSize, TCP::Header header)
    {
        int sent;
        char packet[PACKETSIZE];

        memset(packet, 0, PACKETSIZE);
        memcpy(&packet[0], &(header.seqNum), sizeof(unsigned int));
        memcpy(&packet[4], &(header.ackNum), sizeof(unsigned int));
        memcpy(&packet[8], &(header.flags), sizeof(unsigned short));
        memcpy(&packet[10], &(header.rcwnd), sizeof(unsigned short));

        printf("Sending %d bytes, SEQ=%u ACK=%u FL=%u RCWND=%u\n", dataSize,
               header.seqNum, header.ackNum, header.flags, header.rcwnd);

        memcpy(&packet[HEADERSIZE / sizeof(char)], data, dataSize);

        sent = sendto(sock, packet, HEADERSIZE + dataSize, 0,
                      (const sockaddr *) &si_other, sizeof(si_other));
        return sent;
    }
};


void TCP::setupSockaddrOther(const char * hostname)
{
    /* set up other */
    memset((void* ) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(udpPort);
    if (inet_pton(AF_INET, hostname, &(si_other.sin_addr)) != 1)
        diep("inet_pton() failed\n");
}


TCP::Header TCP::extractHeader(const char *packet){

    TCP::Header header;
    header.seqNum = *((unsigned int *) &packet[0]);
    header.ackNum = *((unsigned int *) &packet[4]);
    header.rcwnd = *((unsigned short *) &packet[6]);
    header.flags = *((unsigned short *) &packet[8]);
    return header;
}


TCP::TCP(unsigned int port) : udpPort(port)
{
    setupSockaddrMe(port);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        diep("socket");

    printf("Now binding to port %d\n", udpPort);
    if (bind(sock, (sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
}


TCP::~TCP()
{
    using namespace std::chrono;
    endtime = high_resolution_clock::now();
    duration<double> time_span =
        duration_cast<duration<double>>(endtime - starttime);
    printf("Closing the socket.\n");
    close(sock);
    cout << "Took " << time_span.count() << " seconds" << endl;
}


void TCP::receiverReceiveData(TCP * tcp, FILE* fp)
{	
    unsigned int bytesRcvdSoFar = 0;
    char packet[PACKETSIZE];
    int recvd;
    TCP::Header header;
    bool shouldWrite = false;

    memset(packet, 0, PACKETSIZE);
    recvd = recvfrom(tcp->sock, packet, PACKETSIZE, 0,
                     (struct sockaddr *)&tcp->si_other, &tcp->slen);
    if(recvd == -1) {
        cout << "Received Error" << endl;
    }

    tcp->starttime = chrono::high_resolution_clock::now();

    while(1)
    {
        /* check SEQ number of packet */
        header = extractHeader(packet);
        cout << "Received " << recvd << " bytes.";
        header.log();

        /* check for FIN */
        if(header.flags & FLAG_FIN)
        {
            printf("Got FIN, going to quit...\n");
            break;
        }

        /* check packet sequencing */
        if(header.seqNum == bytesRcvdSoFar)
        {
            /* packet is in correct sequence */
            bytesRcvdSoFar += (recvd - HEADERSIZE);
            shouldWrite = true;
        } else {
            printf("Got unexpected packet!\n");
        }

        tcp->rcvrACK_mtx.lock();
        tcp->rcvrACK_q.push(bytesRcvdSoFar);
        tcp->rcvrACK_mtx.unlock();

        if(shouldWrite) {
            fwrite(packet + HEADERSIZE, sizeof(char), recvd - HEADERSIZE, fp);
            shouldWrite = false;
        }

        memset(packet, 0, PACKETSIZE);
        recvd = recvfrom(tcp->sock, packet, PACKETSIZE, 0,
                         (struct sockaddr *)&tcp->si_other, &tcp->slen);
        if(recvd == -1) {
            cout << "Received Error" << endl;
            continue;
        }
    }

    header.seqNum = 0;
    header.ackNum = 0;
    header.flags = FLAG_FIN | FLAG_ACK;
    header.rcwnd = 0;
    printf("Sending FINACK!\n");
    tcp->createAndSendPacket(NULL, 0, header);

    fclose(fp);
    tcp->~TCP();
    exit(EXIT_SUCCESS);
}


void TCP::receiverSendACK(TCP * tcp)
{
    while(1)
    {
        unsigned int rcvdSoFar = 0;
        bool shouldACK = false;

        usleep(1000); // sleep for 1 ms
        tcp->rcvrACK_mtx.lock();

        if(tcp->rcvrACK_q.size() > 1)
            cout << "Possible delay..." << endl;

        while(!tcp->rcvrACK_q.empty())
        {
            shouldACK = true;
            unsigned int curr = tcp->rcvrACK_q.front();
            tcp->rcvrACK_q.pop();
            if(curr > rcvdSoFar)
                rcvdSoFar = curr;
        }

        tcp->rcvrACK_mtx.unlock();

        if(shouldACK)
        {
            TCP::Header header;
            header.ackNum = rcvdSoFar;
            header.flags = FLAG_ACK;
            tcp->createAndSendPacket(NULL, 0, header);
        }
    }
}


void TCP::senderReceiveACK(TCP * tcp)
{
}


void TCP::senderSendData(TCP *tcp, FILE *fp, unsigned long long int bytesToTransfer)
{
    char packet[PACKETSIZE];
    unsigned int recvd, sent, recvd_acknum;
    unsigned int totSentSoFar = 0;  // sequence number

    TCP::Header header;
    TCP::Header s_header;
    TCP::Packet to_send;

    fd_set set;
    struct timeval timeout;

    bool shouldIncreaseWindow = false;
    bool resendPackets = false;
    bool stopSending = false;
    bool firstTimeThru = true;

    /* setup start for TCP */
    tcp->sendBase = 0;
    tcp->starttime = chrono::high_resolution_clock::now();

    while (totSentSoFar < bytesToTransfer)
    {
        /* read next part of file */

        /* populate sender congestion window */
        tcp->sender_ctrl.log();
        while (
            header.seqNum >= tcp->sendBase &&
            header.seqNum < (tcp->sendBase + tcp->sender_ctrl.cwnd) &&
            !resendPackets &&
            !stopSending)
        {
            memset(to_send.packet, 0, DATASIZE);
            int toSendThisRound = fread(to_send.packet, sizeof(char), DATASIZE, fp);
            if (toSendThisRound > bytesToTransfer - totSentSoFar)
            {
                /* limit if we have fewer bytes to send than are in the buffer */
                toSendThisRound = bytesToTransfer - totSentSoFar;
                stopSending = true;
            }

            to_send.pkt_length = toSendThisRound;
            tcp->packets_sent[header.seqNum] = &to_send;
            header.seqNum += toSendThisRound;
        }

        /* send all packets in current congestion window */		
        while (!tcp->packets_sent.empty()) 
        {
            for (auto it = tcp->packets_sent.begin(); it != tcp->packets_sent.end(); ++it)
            {
                s_header.seqNum = it->first;
                sent = tcp->createAndSendPacket(
                        it->second->packet, it->second->pkt_length, s_header);
                if(sent != 0)
                {
                    /* start timer for oldest unACKed packet */
                    //if(it == tcp->packets_sent.begin()) start_timer(&it->first);
                    totSentSoFar += sent;
                }
            }

            FD_ZERO(&set);
            FD_SET(tcp->sock, &set);

            /* check for ACK */
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
            select(tcp->sock+1, &set, NULL, NULL, &timeout);
            printf("Took 0.%ld...", (50000-timeout.tv_usec)/1000);

            if(FD_ISSET(tcp->sock, &set))
            {
                recvd = recvfrom(tcp->sock, packet, PACKETSIZE, 0,
                                 (struct sockaddr *) &tcp->si_other, &tcp->slen);

                /* extract ACK # from received packet */
                recvd_acknum = *((unsigned int *) &packet[4]);
                auto min_elem = std::min_element(tcp->packets_sent.begin(),
                                                 tcp->packets_sent.end());

                if(recvd_acknum < min_elem->first)
                {
                    printf("Got unexpected ACK=%u\n", recvd_acknum);
                } else {
                    resendPackets = false;
                    /* loop through packets_sent to see if they are ACKed by cumulative ACK */
                    while(recvd_acknum >= min_elem->first + min_elem->second->pkt_length)
                    {
                        shouldIncreaseWindow = true;
                        tcp->sendBase += min_elem->second->pkt_length;
                        tcp->packets_sent.erase(min_elem);
                        if(tcp->packets_sent.empty()) break;
                        if(!firstTimeThru) 
                            printf("Processing ACK=%u again...\n", recvd_acknum);
                        else 
                            printf("Got ACK=%u\n", recvd_acknum);
                        min_elem = std::min_element(tcp->packets_sent.begin(),
                                                    tcp->packets_sent.end());
                        firstTimeThru = false;
                    } 
                }
                if(shouldIncreaseWindow)
                {
                    tcp->sender_ctrl.cwnd += 1460;
                    shouldIncreaseWindow = false;
                }
            /* TIMEOUT */
            } else {
                resendPackets = true;
                tcp->sender_ctrl.cwnd = 1460;
                printf("No ACK received...\n");
            }
            memset(packet, 0, PACKETSIZE);
            firstTimeThru = true;
        }
    }
}

void TCP::senderCloseConnection()
{
    unsigned int recvd;
    char packet[DATASIZE];
    fd_set set;
    struct timeval timeout;

    /* send FIN packet */
    while(1)
    {
        TCP::Header header;
        header.flags = FLAG_FIN;
        /* send FIN */
        createAndSendPacket(NULL, 0, header);

        FD_ZERO(&set);
        FD_SET(sock, &set);

        /* check for FINACK */
        /* reset timeout to 1 sec for fin */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        select(sock+1, &set, NULL, NULL, &timeout);

        if(FD_ISSET(sock, &set))
        {
            recvd = recvfrom(sock, packet, PACKETSIZE, 0,
                             (struct sockaddr *) &si_other, &slen);

            /* extract FLAGS from received packet */
            TCP::Header gotHeader = TCP::extractHeader(packet);

            printf("Got %d bytes.", recvd);
            gotHeader.log();

            if(gotHeader.flags & (FLAG_FIN | FLAG_ACK))
            {
                printf("Got FINACK!\n");
                break;
            }
        } else {
            /* timeout */
            printf("No FINACK received...\n");
        }
    }
}


#endif // TCP_H
