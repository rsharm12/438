#ifndef TCP_H
#define TCP_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

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

namespace TCP
{
    using namespace std;

    /* timing */
    chrono::high_resolution_clock::time_point starttime;
    chrono::high_resolution_clock::time_point endtime;

    /* stats */
    uint32_t packetsSent;
    uint32_t packetsRecvd;

    /* UDP structures */
    int sock;
    uint16_t udpPort;
    struct sockaddr_in si_me;
    struct sockaddr_in si_other;

    struct Header {
        uint32_t seqNum;
        uint32_t ackNum;
        uint16_t flags;
        uint16_t rcwnd;

        Header();
        Header(uint32_t seqNum, uint32_t ackNum, uint16_t flags, uint16_t rcwnd);
        void log();
    };

    struct Packet {
        Header header;
        char data[DATASIZE];

        Packet(Header header, const char *data, uint32_t dataSize);
        static Header extractHeader(const char *packet);
    };

    class Sender
    {
    private:

    public:
        Sender(uint16_t port);
        ~Sender();

        void sendData(ifstream & fStream, uint64_t bytesToTransfer);
    };

    class Receiver
    {
    private:

    public:
        Receiver(uint16_t port);
        ~Receiver();
    };

    /* functions */
    static void diep(const char *s) {
        perror(s);
        exit(1);
    }

};

#include "tcp.cpp"

#endif // TCP_H