// #include "tcp.h"


namespace TCP
{
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

Packet::Packet(Header header, const char *data, uint32_t dataSize)
{
    this->header = header;
    if (dataSize > DATASIZE)
        dataSize = DATASIZE;
    memcpy(this->data, data, dataSize);
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

}

Receiver::~Receiver()
{
    
}
} // namespace TCP
