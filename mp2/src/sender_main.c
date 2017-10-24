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

struct sockaddr_in si_me;
struct sockaddr_in si_other;
int s;
int sent;
fd_set set;
socklen_t slen = (socklen_t) sizeof(struct sockaddr_in);
int recvd;

unsigned int totPacketsSent = 0;
unsigned int totPacketsRecvd = 0;

char fileBuffer[FILEBUFSIZE];
char buffer[PACKETSIZE];

struct timeval timeout = {
    .tv_sec = 0, .tv_usec = 40000
};


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
    printf("Need to send %lld bytes to %s...\n", bytesToTransfer, hostname);

    setupSockaddrMe(&si_me, hostUDPport);
    s = createAndBindSocket(hostUDPport, &si_me);
    setupSockaddrOther(&si_other, hostname, hostUDPport);

    /* Send data and receive acknowledgements on s */
    unsigned int sentSoFar = 0;  // sequence number
    unsigned int recvd_acknum = 0;  // sequence number
    while(sentSoFar < bytesToTransfer)
    {
        /* read next part of file */
        memset(fileBuffer, 0, FILEBUFSIZE);
        int toSendThisRound = fread(fileBuffer, sizeof(char), FILEBUFSIZE, fp);
        if(toSendThisRound > bytesToTransfer - sentSoFar)
            /* limit if we have fewer bytes to send than are in the buffer */
            toSendThisRound = bytesToTransfer - sentSoFar;

        while(1){
            /* try to send packet */
            struct tcp_header header = {
                .seqNum = sentSoFar,
                .ackNum = 0
            };
            sent = createAndSendPacket(s, fileBuffer, toSendThisRound, &header,
                                       (struct sockaddr *) &si_other);
            if(sent)
                totPacketsSent++;

            FD_ZERO(&set);
            FD_SET(s, &set);

            /* check for ACK */
            select(s+1, &set, NULL, NULL, &timeout);

            if(FD_ISSET(s, &set)){

                recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                         (struct sockaddr *) &si_other, &slen);

                /* extract ACK # from received packet */
                recvd_acknum = *((unsigned int *) &buffer[4]);
                if(recvd_acknum == (sentSoFar + toSendThisRound))
                {
                    printf("Got ACK=%u\n", recvd_acknum);
                    break;
                } else {
                    printf("Got unexpected ACK=%u\n", recvd_acknum);
                }
            } else {
                printf("No ACK received...\n");
            }
        }

        /* packet has been sent and ack'd */
        sentSoFar += toSendThisRound;
        printf("Sent %d bytes so far\n", sentSoFar);
    }

    /* reset timeout to 1 sec for fin */
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    while(1)
    {
        /* send FIN packet */
        struct tcp_header header = {
            .flags = FLAG_FIN,
        };
        sent = createAndSendPacket(s, NULL, 0, &header,
                                (struct sockaddr *) &si_other);
        FD_ZERO(&set);
        FD_SET(s, &set);
        /* check for FINACK */
        select(s+1, &set, NULL, NULL, &timeout);
        if(FD_ISSET(s, &set))
        {
            recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                    (struct sockaddr *) &si_other, &slen);
            /* extract FLAGS from received packet */
            unsigned short flags = *((unsigned short *) &buffer[8]);
            if(flags & (FLAG_FIN | FLAG_ACK))
            {
                printf("Got FINACK\n");
                break;
            }
        }
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
