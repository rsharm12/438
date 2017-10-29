// #include "tcp.h"


namespace TCP
{

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

int UDP::recv(char *buffer, uint32_t dataSize)
{
    return recvfrom(sock, buffer, dataSize, 0,
                     (struct sockaddr *)&si_other, &slen_other);
}

int UDP::send(const char * packet, int packetSize)
{
    return sendto(sock, packet, packetSize, 0,
                  (const sockaddr *) &si_other, sizeof(si_other));
}

/* Header */

Header::Header() : seqNum(0), ackNum(0), flags(0), rcwnd(0)
{
}

Header::Header(uint32_t seqNum, uint32_t ackNum, uint16_t flags, uint16_t rcwnd) :
    seqNum(seqNum), ackNum(ackNum), flags(flags), rcwnd(rcwnd)
{
}

void Header::log()
{
    cout << " SEQ=" << seqNum;
    cout << " ACK=" << ackNum;
    cout << " FL="  << flags;
    cout << " RW="  << rcwnd << endl;
}

/* Packet */
Packet::Packet()
{
    dataSize = 0;
    memset(data, 0, DATASIZE);
}

Packet::Packet(Header header, const char *data, uint32_t dataSize)
{
    this->header = header;
    if (dataSize > DATASIZE)
        this->dataSize = DATASIZE;
    else
        this->dataSize = dataSize;
    memcpy(this->data, data, this->dataSize);
}


void Packet::update(const char *buffer, uint32_t bufferSize)
{
    this->header = extractHeader(buffer);
    memset(this->data, 0, DATASIZE);
    if (bufferSize > DATASIZE)
        this->dataSize = DATASIZE;
    else
        this->dataSize = bufferSize;
    memcpy(this->data, buffer, this->dataSize);
}


void Packet::toBuffer(char *buffer)
{
    memset(buffer, 0, PACKETSIZE);
    memcpy(&buffer[0], &(header.seqNum), sizeof(uint32_t));
    memcpy(&buffer[4], &(header.ackNum), sizeof(uint32_t));
    memcpy(&buffer[8], &(header.flags), sizeof(uint16_t));
    memcpy(&buffer[10], &(header.rcwnd), sizeof(uint16_t));
    memcpy(&buffer[HEADERSIZE / sizeof(char)], data, this->dataSize);

}


Header Packet::extractHeader(const char *packet)
{
    return Header(*((uint32_t *) &packet[0]),
                  *((uint32_t *) &packet[4]),
                  *((uint16_t *) &packet[6]),
                  *((uint16_t *) &packet[8]));
}

/* Sender */

Sender::Sender(uint16_t port)
{
    udpPort = port;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        diep("Socket create error");

    if (bind(sock, (sockaddr*) &si_me, sizeof (si_me)) == -1)
        diep("bind");
}

Sender::~Sender()
{
    using namespace std::chrono;

    cout << "Closing socket" << endl;
    close(sock);

    endtime = high_resolution_clock::now();
    duration<double> time_span =
        duration_cast<duration<double>>(endtime - starttime);

    cout << "Took " << time_span.count() << " seconds" << endl;
    cout << "Packet Stats: Sent=" << packetsSent;
    cout << " Recvd=" << packetsRecvd << endl;
}

void Sender::sendData(ifstream & fStream, uint64_t bytesToTransfer)
{
    starttime = chrono::high_resolution_clock::now();
}

/* Receiver */

Receiver::Receiver(uint16_t port)
{
    /* TODO: set up sockaddrme */
    

}

Receiver::~Receiver()
{
    using namespace std::chrono;

    cout << "Closing socket" << endl;
    close(sock);

    endtime = high_resolution_clock::now();
    duration<double> time_span =
        duration_cast<duration<double>>(endtime - starttime);

    cout << "Took " << time_span.count() << " seconds" << endl;
    cout << "Packet Stats: Sent=" << packetsSent;
    cout << " Recvd=" << packetsRecvd << endl;

}

void Receiver::receiveData(UDP & udp, ofstream & fStream, mutex & rcvrACK_mtx,
                           queue<uint32_t> & rcvrACK_q)
{
    int recvd;
    uint32_t bytesRcvdSoFar = 0;
    char buffer[PACKETSIZE];
    bool shouldWrite = false;
    Header header;
    Packet packet;

    memset(buffer, 0, PACKETSIZE);
    recvd = udp.recv(buffer, PACKETSIZE);
    if(recvd == -1) {
        cout << "Received Error" << endl;
    }

    starttime = chrono::high_resolution_clock::now();

    while(1)
    {
        packet.update(buffer, recvd);
        TCP::packetsRecvd++;

        /* check SEQ number of packet */
        cout << "Received " << recvd << " bytes.";
        packet.header.log();

        /* check for FIN */
        if(packet.header.flags & FLAG_FIN)
        {
            cout << "Got FIN, going to quit..." << endl;
            break;
        }

        /* check packet sequencing */
        if(packet.header.seqNum == bytesRcvdSoFar)
        {
            /* packet is in correct sequence */
            bytesRcvdSoFar += (recvd - HEADERSIZE);
            shouldWrite = true;
        } else {
            cout << "Got unexpected packet!" << endl;
        }

        rcvrACK_mtx.lock();
        rcvrACK_q.push(bytesRcvdSoFar);
        rcvrACK_mtx.unlock();

        /* write to file */
        if(shouldWrite) {
            fStream.write(packet.data, recvd - HEADERSIZE);
            shouldWrite = false;
        }

        memset(buffer, 0, PACKETSIZE);
        recvd = udp.recv(buffer, PACKETSIZE);

        if(recvd == -1) {
            cout << "Received Error!!!!!!!!!!!!" << endl;
            break;
        }

    }

    Packet fin_ack(Header(0, 0, FLAG_FIN | FLAG_ACK, 0), nullptr, 0);
    fin_ack.toBuffer(buffer);
    cout << "Sending FINACK!" << endl;
    udp.send(buffer, HEADERSIZE);

    // exit(EXIT_SUCCESS);
}


void Receiver::sendACK(UDP & udp, mutex & rcvrACK_mtx, queue<uint32_t> & rcvrACK_q)
{
    char buffer[PACKETSIZE];

    while(1)
    {
        uint32_t rcvdSoFar = 0;
        bool shouldACK = false;

        usleep(1000); // sleep for 1 ms
        rcvrACK_mtx.lock();

        if(rcvrACK_q.size() > 1)
            cout << "Possible delay..." << endl;

        while(!rcvrACK_q.empty())
        {
            shouldACK = true;
            uint32_t curr = rcvrACK_q.front();
            rcvrACK_q.pop();
            if(curr > rcvdSoFar)
                rcvdSoFar = curr;
        }

        rcvrACK_mtx.unlock();

        if(shouldACK)
        {
            Packet ack_pkt(Header(0, rcvdSoFar, FLAG_ACK, 0), nullptr, 0);
            ack_pkt.toBuffer(buffer);
            cout << "Sending ACK... ";
            ack_pkt.header.log();
            udp.send(buffer, HEADERSIZE);
        }
    }
}




} // namespace TCP
