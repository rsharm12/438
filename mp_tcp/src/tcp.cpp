#include "tcp.h"

namespace TCP
{

/* UDP */

UDP::UDP(uint16_t udpPort)
{
    /* set up me */
    memset((void *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(udpPort);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        TCP::diep("Socket create error");

    if (bind(sock, (sockaddr*) &si_me, sizeof (si_me)) == -1)
        TCP::diep("bind");
}

UDP::UDP(uint16_t udpPort, string hostname)
{
    /* set up me */
    memset((void *) &si_me, 0, sizeof (si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(udpPort);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        TCP::diep("Socket create error");

    if (bind(sock, (sockaddr*) &si_me, sizeof (si_me)) == -1)
        TCP::diep("bind");

    /* set up other */
    memset((void *) &si_other, 0, sizeof (si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(udpPort);
    if (inet_pton(AF_INET, hostname.c_str(), &(si_other.sin_addr)) != 1)
        diep("inet_pton() failed");
}

UDP::~UDP()
{
    cout << "Closing socket" << endl;
    close(sock);
}

int UDP::recv(char *buffer, uint32_t dataSize, int flags, bool shouldSave)
{
    int recv;

    if(shouldSave)
        recv = recvfrom(sock, buffer, dataSize, flags,
                        (struct sockaddr *)&si_other, &slen_other);
    else
        recv = recvfrom(sock, buffer, dataSize, flags,
                        (struct sockaddr *)&si_tmp, &slen_tmp);

    if(-1 == recv)
        perror("UDP::recv ERROR");
    return recv;
}

int UDP::send(const char *packet, int packetSize, int flags)
{
    int sent = sendto(sock, packet, packetSize, flags,
                      (const sockaddr *) &si_other, slen_other);
    if(-1 == sent)
        perror("UDP::send ERROR");
    return sent;
}

/* Header */

Header::Header() : seqNum(0), ackNum(0), dataLen(0), flags(0), rcwnd(0)
{
}

Header::Header(uint32_t seqNum, uint32_t ackNum, uint32_t dataLen,
               uint16_t flags, uint16_t rcwnd) :
    seqNum(seqNum), ackNum(ackNum), dataLen(dataLen), flags(flags), rcwnd(rcwnd)
{
}

void Header::log()
{
    cout << " SEQ=" << seqNum;
    cout << " ACK=" << ackNum;
    cout << " DLEN=" << dataLen;
    cout << " FL="  << flags;
    cout << " RW="  << rcwnd << endl;
}

/* Packet */

Packet::Packet()
{
    memset(data, 0, DATASIZE);
}

Packet::Packet(Header header, const char *data)
{
    this->header = header;
    if(this->header.dataLen > DATASIZE)
        this->header.dataLen = DATASIZE;
    memcpy(this->data, data, this->header.dataLen);
}

void Packet::update(const char *buffer, uint32_t bufferSize)
{
    this->header = extractHeader(buffer);
    memset(this->data, 0, DATASIZE);
    if (bufferSize > DATASIZE)
        this->header.dataLen = DATASIZE;
    else
        this->header.dataLen = bufferSize - HEADERSIZE;
    memcpy(this->data, buffer+HEADERSIZE, this->header.dataLen);
}

void Packet::toBuffer(char *buffer)
{
    memset(buffer, 0, PACKETSIZE);
    memcpy(&buffer[0], &(header.seqNum), sizeof(uint32_t));
    memcpy(&buffer[4], &(header.ackNum), sizeof(uint32_t));
    memcpy(&buffer[8], &(header.dataLen), sizeof(uint32_t));
    memcpy(&buffer[12], &(header.flags), sizeof(uint16_t));
    memcpy(&buffer[14], &(header.rcwnd), sizeof(uint16_t));
    memcpy(&buffer[HEADERSIZE / sizeof(char)], data, header.dataLen);
}

Header Packet::extractHeader(const char *packet)
{
    return Header(*((uint32_t *) &packet[0]),
                  *((uint32_t *) &packet[4]),
                  *((uint32_t *) &packet[8]),
                  *((uint16_t *) &packet[12]),
                  *((uint16_t *) &packet[14]));
}

/* CongestionControl */

CongestionControl::CongestionControl(uint64_t totalSize) : totalSize(totalSize)
{
    currState = SLOW_START;
    isDone = false;

    f_size = (double) DATASIZE;
    size = DATASIZE;
    ssthresh = 65536; // 64 KB
    dupACKcount = 0;
    sendBase = 0;
    nextSeqNum = 0;
}

void CongestionControl::log()
{
    switch(currState)
    {
        case SLOW_START: cout << "S"; break;
        case CNG_AVOID: cout << "C"; break;
        case FAST_REC: cout << "F"; break;
    }
    cout << " size=" << size;
    cout << " f_size=" << std::setprecision(2) << std::fixed << f_size;
    cout << " ssthresh=" << ssthresh;
    cout << " dupACK="  << dupACKcount;
    cout << " sendBase="  << sendBase;
    cout << " nextSeqNum=" << nextSeqNum;
    cout << " cwnd=" << cwnd.size() << endl;
}

/* Sender */

void Sender::setupConnection(UDP * udp)
{
    int recvd;
    char buffer[PACKETSIZE];
    fd_set set;
    struct timeval timeout;

    Packet packet(Header(0, 0, FLAG_SYN, 0), nullptr, 0);

    // timeout.tv_sec = 1;
    // timeout.tv_usec = 0;

    // setsockopt(udp->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* send SYN packet */
    while(1)
    {
        packet.toBuffer(buffer);

        /* send SYN */
        cout << "Sending SYN...";
        packet.header.log();
        udp->send(buffer, HEADERSIZE);

        FD_ZERO(&set);
        FD_SET(udp->sock, &set);

        /* check for SYNACK */
        /* reset timeout to 1 sec for syn */
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        select(udp->sock+1, &set, NULL, NULL, &timeout);

        memset(buffer, 0, PACKETSIZE);

        if(FD_ISSET(udp->sock, &set))
        {
            recvd = udp->recv(buffer, PACKETSIZE);

            /* extract FLAGS from received packet */
            Header gotHeader = Packet::extractHeader(buffer);

            cout << "Got " << recvd << " bytes.";
            gotHeader.log();

            if(gotHeader.flags & (FLAG_SYN | FLAG_ACK))
            {
                cout << "Got SYNACK!" << endl;
                break;
            }
        } else {
            /* timeout */
            cout << "No SYNACK received..." << endl;
        }
    }

    // Packet ack(Header(0, 0, FLAG_ACK, 0), nullptr, 0);
    // ack.toBuffer(buffer);
    // cout << "Sending (SYN)ACK!" << endl;
    // udp->send(buffer, HEADERSIZE);
}

void Sender::sendData(UDP * udp, CongestionControl * cc)
{
    char buffer[PACKETSIZE];

    while(1)
    {
        cc->cc_mtx.lock();

        if(cc->isDone)
            break;

        /* data to be sent from cwnd */
        for(; cc->nextSeqNum < (cc->sendBase + cc->size); cc->nextSeqNum += DATASIZE)
        {
            if(cc->cwnd.find(cc->nextSeqNum) == cc->cwnd.end())
                /* packet not loaded */
                break;

            Packet packet = cc->cwnd[cc->nextSeqNum];
            packet.toBuffer(buffer);

            cout << "\nSending " << packet.dataSize << " bytes. ";
            packet.header.log();
            cout << "sendData: ";
            cc->log();

            udp->send(buffer, packet.header.dataLen + HEADERSIZE, 0);
            TCP::packetsSent++;
        }

        cc->cc_mtx.unlock();
        usleep(100);
    }

    cc->cc_mtx.unlock();
    cout << "sendData is done!" << endl;
}

void Sender::recvACK(UDP * udp, CongestionControl * cc)
{
    int recvd;
    char buffer[PACKETSIZE];
    fd_set set;
    struct timeval timeout;

    while(1)
    {
        usleep(1000);

        FD_ZERO(&set);
        FD_SET(udp->sock, &set);

        cc->cc_mtx.lock();

        /* check for ACK */
        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;
        select(udp->sock+1, &set, NULL, NULL, &timeout);
        cout <<  "Took " << (50000-timeout.tv_usec)/1000 << "ms" << endl;

        if(FD_ISSET(udp->sock, &set))
        {
            memset(buffer, 0, PACKETSIZE);
            recvd = udp->recv(buffer, PACKETSIZE);

            Header header = Packet::extractHeader(buffer);
            cout << "Received " << recvd << " bytes.";
            header.log();

            /* errant SYNACK packet */
            if(header.flags | (FLAG_SYN | FLAG_ACK)){
                cout << "ERROR SYNACK" << endl;
                cc->cc_mtx.unlock();
                continue;
            }

            TCP::packetsRecvd++;

            if(header.ackNum >= cc->totalSize)
            {
                cc->isDone = true;
                break;
            }

            if(header.ackNum < cc->sendBase)
            {
                /* duplicate ACK received */
                cout << "Duplicate ACK received." << endl;


                cc->dupACKcount++;
                if(cc->dupACKcount == 3 && cc->currState != FAST_REC)
                {
                    /* switching to Fast Recovery */
                    cout << "Switching to fast recovery..." << endl;
                    cc->currState = FAST_REC;
                    cc->ssthresh = cc->size/2;
                    cc->size += 3*DATASIZE;
                    cc->f_size = cc->size;
                    cc->nextSeqNum = cc->sendBase;
                }
            }
            else
            {
                /* new ACK received */
                cout << "New ACK received." << endl;

                /* remove all ACKed packets from map */
                for(; cc->sendBase < header.ackNum; cc->sendBase += DATASIZE)
                {
                    cout << "Trying to remove " << cc->sendBase;
                    if(cc->cwnd.find(cc->sendBase) != cc->cwnd.end())
                    {
                        /* erase only if present in map */
                        cc->cwnd.erase(cc->sendBase);
                        cout << "... yes!" << endl;
                    } else {
                        cout << "... nope!" << endl;
                    }
                }

                /* should be true:
                 * cc->sendBase = header.ackNum;
                 */
                if(cc->sendBase != header.ackNum)
                    cout << "sendBase != ackNum" << endl;

                if(cc->nextSeqNum < cc->sendBase)
                    cc->nextSeqNum = cc->sendBase;

                switch(cc->currState)
                {
                case SLOW_START:
                    cc->size += DATASIZE;
                    cc->f_size = cc->size;
                    cc->dupACKcount = 0;
                    if(cc->size >= cc->ssthresh)
                    {
                        cc->currState = CNG_AVOID;
                        cout << "Switching to CNG_AVOID..." << endl;
                    }
                    break;
                case CNG_AVOID:
                    cc->f_size += DATASIZE * (DATASIZE/cc->f_size);
                    cc->size = floor(cc->f_size);
                    cc->dupACKcount = 0;
                    break;
                case FAST_REC:
                    cc->currState = CNG_AVOID;
                    cc->size = cc->ssthresh;
                    cc->f_size = cc->size;
                    cc->dupACKcount = 0;
                    break;
                }
            }
        }
        else{
            /* TIMEOUT: restart timer for any ACK */
            cout << "Timeout!" << endl;

            senderTimeouts++;
            cc->currState = SLOW_START;
            cc->ssthresh = cc->size/2;
            cc->size = DATASIZE;
            cc->f_size = cc->size;
            cc->dupACKcount = 0;
            cc->nextSeqNum = cc->sendBase;
        }

        cout << "recvACK: ";
        cc->log();
        cc->cc_mtx.unlock();
    }

    cc->cc_mtx.unlock();

    cout << "recvACK is done!" << endl;
}

void Sender::updateWindow(CongestionControl * cc, ifstream * fStream)
{
    uint64_t readSoFar = 0;
    bool shouldLog = false;

    while(readSoFar < cc->totalSize)
    {
        cc->cc_mtx.lock();

        /* data to be read and placed in cwnd */
        for(uint32_t seqNum = cc->nextSeqNum;
            seqNum < (cc->sendBase + cc->size);
            seqNum += DATASIZE)
        {
            if(cc->cwnd.find(seqNum) != cc->cwnd.end())
                /* packet already exists */
                continue;

            Packet packet;
            fStream->read(packet.data, DATASIZE);
            packet.header.dataLen = fStream->gcount();
            cout << "updateWindow: read " << packet.header.dataLen << " bytes." << endl;
            readSoFar += packet.header.dataLen;

            packet.header.seqNum = seqNum;
            cc->cwnd[seqNum] = packet;

            shouldLog = true;
        }

        if(shouldLog) {
            cout << "updateWindow: ";
            cc->log();
            shouldLog = false;
        }

        cc->cc_mtx.unlock();
        usleep(100);
    }

    cout << "updateWindow is done!" << endl;
}

void Sender::closeConnection(UDP * udp)
{
    int recvd;
    char buffer[PACKETSIZE];
    fd_set set;
    struct timeval timeout;

    /* send FIN packet */
    while(1)
    {
        Packet packet(Header(0, 0, 0, FLAG_FIN, 0), nullptr, 0);
        packet.toBuffer(buffer);

        /* send FIN */
        cout << "Sending FIN...";
        packet.header.log();
        udp->send(buffer, HEADERSIZE);

        FD_ZERO(&set);
        FD_SET(udp->sock, &set);

        /* check for FINACK */
        /* reset timeout to 1 sec for fin */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        select(udp->sock+1, &set, NULL, NULL, &timeout);

        memset(buffer, 0, PACKETSIZE);

        if(FD_ISSET(udp->sock, &set))
        {
            recvd = udp->recv(buffer, HEADERSIZE, false);

            /* extract FLAGS from received packet */
            Header gotHeader = Packet::extractHeader(buffer);

            cout << "Got " << recvd << " bytes.";
            gotHeader.log();

            if(gotHeader.flags & (FLAG_FIN | FLAG_ACK))
            {
                cout << "Got FINACK!" << endl;
                break;
            }
        } else {
            /* timeout */
            cout << "No FINACK received..." << endl;
        }
    }
}

/* Receiver */

void Receiver::waitForConnection(UDP * udp)
{
    int recvd;
    char buffer[PACKETSIZE];

    while(1)
    {
        memset(buffer, 0, PACKETSIZE);
        recvd = udp->recv(buffer, PACKETSIZE, true);
        /* extract FLAGS from received packet */
        Header gotHeader = Packet::extractHeader(buffer);

        cout << "Got " << recvd << " bytes.";
        gotHeader.log();

        if(gotHeader.flags & (FLAG_SYN))
        {
            cout << "Got SYN!" << endl;
            break;
        }
    }

    Packet synack(Header(0, 0, FLAG_SYN | FLAG_ACK, 0), nullptr, 0);
    synack.toBuffer(buffer);
    cout << "Sending SYNACK!" << endl;
    /* send 3 SYNACKs in case of dropped packets */
    udp->send(buffer, HEADERSIZE);
    udp->send(buffer, HEADERSIZE);
    udp->send(buffer, HEADERSIZE);
}

void Receiver::receiveData(UDP *udp, ofstream *fStream,
                           mutex *rcvrACK_mtx, queue<uint32_t> *rcvrACK_q)
{
    int recvd = -1;
    uint32_t bytesRcvdSoFar = 0;

    char buffer[PACKETSIZE];
    Header header;
    Packet packet;

    while(recvd == -1)
    {
        memset(buffer, 0, PACKETSIZE);
        recvd = udp->recv(buffer, PACKETSIZE, MSG_PEEK, true);
    }

    TCP::starttime = high_resolution_clock::now();

    while(1)
    {
        memset(buffer, 0, PACKETSIZE);
        recvd = udp->recv(buffer, PACKETSIZE, 0, false);
        if(recvd == -1) {
            cout << "Received Error!!!!!!!!!!!!" << endl;
            continue;
        }

        packet.update(buffer, recvd);
        TCP::packetsRecvd++;

        /* check SEQ number of packet */
        cout << "Received " << recvd << " bytes.";
        packet.header.log();

        /* check for FIN */
        if(packet.header.flags == FLAG_FIN)
        {
            cout << "Got FIN, going to quit..." << endl;
            break;
        }

        /* check packet sequencing */
        if(packet.header.seqNum == bytesRcvdSoFar)
        {
            /* packet is in correct sequence */
            bytesRcvdSoFar += (recvd - HEADERSIZE);
            /* write to file */
            if(shouldWrite)
            {
                fStream->write(packet.data, packet.header.dataLen);
                shouldWrite = false;
            }
        } else if(packet.header.seqNum < bytesRcvdSoFar) {
            cout << "Got duplicate packet!" << endl;
        } else {
            cout << "Got unexpected packet!" << endl;
        }

        rcvrACK_mtx->lock();
        rcvrACK_q->push(bytesRcvdSoFar);
        rcvrACK_mtx->unlock();
    }

    Packet fin_ack(Header(0, 0, 0, FLAG_FIN | FLAG_ACK, 0), nullptr, 0);
    fin_ack.toBuffer(buffer);
    cout << "Sending FINACK!" << endl;
    udp->send(buffer, HEADERSIZE, 0);
}


void Receiver::sendACK(UDP *udp, mutex *rcvrACK_mtx, queue<uint32_t> *rcvrACK_q)
{
    using namespace chrono;

    char buffer[PACKETSIZE];
    high_resolution_clock::time_point last_ACK_time;

    while(1)
    {
        uint32_t largestAckInQueue = 0;
        bool shouldACK = false;

        usleep(1000); // sleep for 1 ms

        rcvrACK_mtx->lock();
        if(rcvrACK_q->size() > 1)
            cout << "Possible delay..." << endl;

        while(!rcvrACK_q->empty())
        {
            shouldACK = true;
            uint32_t curr = rcvrACK_q->front();
            rcvrACK_q->pop();
            if(curr > largestAckInQueue)
                largestAckInQueue = curr;
        }

        rcvrACK_mtx->unlock();

        if(shouldACK)
        {
            last_ACK_time = high_resolution_clock::now();
            Packet ack_pkt(Header(0, largestAckInQueue, 0, FLAG_ACK, 0), nullptr, 0);
            ack_pkt.toBuffer(buffer);
            cout << "Sending ACK... ";
            ack_pkt.header.log();
            udp->send(buffer, HEADERSIZE, 0);
            TCP::packetsSent++;
        }

        high_resolution_clock::time_point now = high_resolution_clock::now();
        duration<double> time_span =
            duration_cast<duration<double>>(now - last_ACK_time);
        if(time_span > seconds(TIMEOUT) && TCP::packetsSent > 0) break;
    }

    cout << "sendACK: done" << endl;
}

} // namespace TCP
