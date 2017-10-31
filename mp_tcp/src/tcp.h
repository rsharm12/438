#ifndef TCP_H
#define TCP_H

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <cmath>

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
#define HEADERSIZE  16
#define DATASIZE    (PACKETSIZE - HEADERSIZE)
/*
 * HEADER:
 *        4 bytes - seq number
 *        4 bytes - ack number
 *        4 bytes - data length
 *        2 bytes - flags
 *        2 bytes - rcv window
 * DATA:
 *        up to 1456 bytes - data
 */

 /* TCP FLAG BITMAP */
#define FLAG_ACK   0x1
#define FLAG_FIN   0x2
#define FLAG_SYN   0x4

#define TIMEOUT    1

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
    struct UDP {
        int sock;
        // uint16_t udpPort;
        struct sockaddr_in si_me;
        struct sockaddr_in si_other;
        struct sockaddr_storage si_tmp;
        socklen_t slen_me = (socklen_t) sizeof(struct sockaddr_in);
        socklen_t slen_other = (socklen_t) sizeof(struct sockaddr_in);
        socklen_t slen_tmp = (socklen_t) sizeof(struct sockaddr_storage);

        UDP(uint16_t udpPort);
        UDP(uint16_t udpPort, string hostname);
        ~UDP();
        int recv(char *buffer, uint32_t dataSize, bool shouldSave);
        int send(const char *packet, int packetSize);
    };

    struct Header {
        uint32_t seqNum;
        uint32_t ackNum;
        uint32_t dataLen;
        uint16_t flags;
        uint16_t rcwnd;

        Header();
        Header(uint32_t seqNum, uint32_t ackNum, uint32_t dataLen,
               uint16_t flags, uint16_t rcwnd);
        void log();
    };

    struct Packet {
        Header header;
        char data[DATASIZE];

        Packet();
        Packet(Header header, const char *data);
        void update(const char *buffer, uint32_t bufferSize);
        void toBuffer(char *buffer);
        static Header extractHeader(const char *buffer);
    };

    enum CCState { SLOW_START, CNG_AVOID, FAST_REC };

    struct CongestionControl {
        mutex cc_mtx;
        CCState currState;
        bool isDone;

        double f_size;
        uint64_t size;            // size of window in number of packets
        uint64_t ssthresh;        // threshold in bytes
        uint32_t dupACKcount;     // counter for duplicate ACKs
        uint64_t sendBase;        // first SEQ num that is sent not ACKed
        uint64_t nextSeqNum;      // first SEQ num that has not been sent
        uint64_t totalSize;       // every number should be <= to totalSize

        unordered_map<uint64_t, Packet> cwnd;

        CongestionControl(uint64_t totalSize);
        void log();
    };


    namespace Sender
    {
        uint32_t senderTimeouts;
        static void setupConnection(UDP * udp);
        static void sendData(UDP * udp, CongestionControl * cc);
        static void recvACK(UDP * udp, CongestionControl * cc);
        static void updateWindow(CongestionControl *cc, ifstream * fStream);
        static void closeConnection(UDP * udp);
    };

    namespace Receiver
    {
        static void waitForConnection(UDP * udp);
        static void receiveData(UDP *udp, ofstream *fStream, mutex *rcvrACK_mtx, queue<uint32_t> *rcvrACK_q);
        static void sendACK(UDP *udp, mutex *rcvrACK_mtx, queue<uint32_t> *rcvrACK_q);
    };

    /* functions */
    static void diep(const char *s) {
        perror(s);
        exit(1);
    }

};

#include "tcp.cpp"

#endif // TCP_H