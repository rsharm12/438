/* 
 * File:   sender_main.c
 * Author: 
 *
 * Created on 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include "tcp.h"

#define PACKETSIZE  1472
#define HEADERSIZE  8
#define FILEBUFSIZE (PACKETSIZE - HEADERSIZE)
/*
 * HEADER:
 *        4 bytes - seq number
 *        4 bytes - ack number
 * DATA:
 *        up to 1464 bytes - data
 */

struct sockaddr_in si_me;
struct sockaddr_in si_other;
int s;
socklen_t slen = (socklen_t) sizeof(struct sockaddr_in);
int recvd;

unsigned int totPacketsSent = 0;
unsigned int totPacketsRecvd = 0;

char fileBuffer[FILEBUFSIZE];

void createAndSendPacket(const char * fileBuf, int bufSize, unsigned int seqNum,
                         unsigned int * expectedAckNum)
{
    int sent;
    char packet[PACKETSIZE];

    memcpy(&packet[0], &seqNum, sizeof(unsigned int));
    memcpy(&packet[4], expectedAckNum, sizeof(unsigned int));
    memcpy(&packet[HEADERSIZE / sizeof(char)], fileBuf, bufSize);

    printf("Sending %d bytes, SEQ=%d ACK=%d. ", bufSize, seqNum, *expectedAckNum);

    sent = sendto(s, packet, HEADERSIZE + bufSize, 0,
                  (struct sockaddr *) &si_other, slen);
    totPacketsSent++;
}

void reliablyTransfer(char* hostname, unsigned short int hostUDPport,
                      char* filename, unsigned long long int bytesToTransfer)
{
    /* Open the file */
    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
        perror("Could not open file to send.\n");

    /* Determine how many bytes to transfer */
    fseek(fp, 0L, SEEK_END);
    int fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if(bytesToTransfer > fileSize)
        bytesToTransfer = fileSize;
    printf("Need to sent %lld bytes to %s...\n", bytesToTransfer, hostname);

    setupSockaddrMe(&si_me, hostUDPport);
    s = createAndBindSocket(hostUDPport, &si_me);
    setupSockaddrOther(&si_other, hostname, hostUDPport);

    /* Send data and receive acknowledgements on s */
    unsigned int sentSoFar = 0;  // sequence number
    unsigned int expectedAck = 0;  // ack number
    while(sentSoFar < bytesToTransfer)
    {
        /* read next part of file */
        memset(fileBuffer, 0, FILEBUFSIZE);
        int toSendThisRound = fread(fileBuffer, sizeof(char), FILEBUFSIZE, fp);
        if(toSendThisRound > bytesToTransfer - sentSoFar)
            /* limit if we have fewer bytes to send than are in the buffer */
            toSendThisRound = bytesToTransfer - sentSoFar;

        /* try to send packet */
        createAndSendPacket(fileBuffer, toSendThisRound, sentSoFar,
                            &expectedAck);

        /* check for ACK */

        /* packet has been sent and ack'd */
        sentSoFar += toSendThisRound;
        printf("Sent %d bytes so far\n", sentSoFar);
    }

    printf("Closing the socket and file\n");
    close(s);
    fclose(fp);
}

/*
 * 
 */
int main(int argc, char** argv)
{
    unsigned short int udpPort;
    unsigned long long int numBytes;

    if (argc != 5)
    {
        fprintf(stderr, "usage: %s receiver_hostname receiver_port "
                "filename_to_xfer bytes_to_xfer\n\n", argv[0]);
        exit(1);
    }
    udpPort = (unsigned short int) atoi(argv[2]);
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);
    printf("\nPacket stats: Recvd %d Sent %d\n", totPacketsRecvd, totPacketsSent);

    return (EXIT_SUCCESS);
}
