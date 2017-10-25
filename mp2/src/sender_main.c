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
#include <fcntl.h>

#include "tcp.h"

struct timespec startspec;
struct timespec endspec;
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

struct timeval timeout;
struct timeval f_timeout;

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

    clock_gettime(CLOCK_REALTIME, &startspec);

    setupSockaddrMe(&si_me, hostUDPport);
    s = createAndBindSocket(hostUDPport, &si_me);
    setupSockaddrOther(&si_other, hostname, hostUDPport);
    int opts = fcntl(s, F_GETFL);

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

        while(1)
        {
            memset(buffer, 0, PACKETSIZE);
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
            timeout.tv_sec = 0;
            timeout.tv_usec = 50000;
            select(s+1, &set, NULL, NULL, &timeout);
            printf("Took 0.%ld...", (50000-timeout.tv_usec)/1000);

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
        printf("Sent %d bytes so far\n\n", sentSoFar);
    }

    clock_gettime(CLOCK_REALTIME, &endspec);
    unsigned int sec = endspec.tv_sec - startspec.tv_sec;
    unsigned int msec = (endspec.tv_nsec - startspec.tv_nsec) / 1000000;
    printf("\nTime taken = %u.%u\n", sec, msec);

    printf("\nFile sent, trying to close connection...\n");

    usleep(500000);

    /* empty out the socket read buffer */
    opts = opts | O_NONBLOCK;
    fcntl(s, F_SETFL, opts);
    while(-1 != recvfrom(s, buffer, PACKETSIZE, 0, (struct sockaddr *) &si_other, &slen));

    opts = opts & ~O_NONBLOCK;
    fcntl(s, F_SETFL, opts);

    while(1)
    {
        memset(buffer, 0, PACKETSIZE);
        /* send FIN packet */
        struct tcp_header header = {
            .flags = FLAG_FIN,
        };
        sent = createAndSendPacket(s, NULL, 0, &header, (struct sockaddr *) &si_other);

        FD_ZERO(&set);
        FD_SET(s, &set);

        /* check for FINACK */
        /* reset timeout to 1 sec for fin */
        f_timeout.tv_sec = 1;
        f_timeout.tv_usec = 0;
        select(s+1, &set, NULL, NULL, &f_timeout);
        if(FD_ISSET(s, &set))
        {
            recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                    (struct sockaddr *) &si_other, &slen);
            /* extract FLAGS from received packet */
            int finseq = *((unsigned int *) &buffer[0]);
            int finack = *((unsigned int *) &buffer[4]);
            unsigned short flags = *((unsigned short *) &buffer[8]);
            printf("Got %d bytes, SEQ=%d ACK=%d FL=%u\n", recvd, finseq, finack, flags);
            if(flags & (FLAG_FIN | FLAG_ACK))
            {
                printf("Got FINACK\n");
                break;
            }
        } else {
            printf("No FINACK received...\n");
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
