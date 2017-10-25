/* 
 * File:   receiver_main.c
 * Author: 
 *
 * Created on
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "tcp.h"

struct timespec startspec;
struct timespec endspec;
struct sockaddr_in si_me;
struct sockaddr_in si_other;
int s;
socklen_t slen = (socklen_t) sizeof(struct sockaddr_in);
int recvd;
char buffer[PACKETSIZE];
char ipaddr[INET_ADDRSTRLEN];
char header[HEADERSIZE];

unsigned int totPacketsSent = 0;
unsigned int totPacketsRecvd = 0;

void reliablyReceive(unsigned short int myUDPport, char* destinationFile)
{
    setupSockaddrMe(&si_me, myUDPport);
    s = createAndBindSocket(myUDPport, &si_me);
    int opts = fcntl(s, F_GETFL);

    FILE *fp;
    fp = fopen(destinationFile, "wb");
    if (fp == NULL)
        diep("Could not open file to save.\n");

    unsigned int currentSeqNum = 0;
    unsigned int currentAckNum = 0;
    unsigned int recvd_seqnum, recvd_acknum;

    /* Now receive data and send acknowledgements */
    memset(buffer, 0, PACKETSIZE);
    recvd = recvfrom(s, buffer, PACKETSIZE, 0, (struct sockaddr *)&si_other, &slen);

    clock_gettime(CLOCK_REALTIME, &startspec);
    
    memset(ipaddr, 0, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(((struct sockaddr_in *)&si_other)->sin_addr), ipaddr, slen);
    printf("Got data from %s\n", ipaddr);

    /* set to non-blocking socket after 1st receive */
   // opts = opts | O_NONBLOCK;
   // fcntl(s, F_SETFL, opts);

    while(recvd > 0)
    {
        /* copy header to new buffer */
        recvd_seqnum = *((unsigned int *) &buffer[0]);
        unsigned short flags = *((unsigned short *) &buffer[8]);

        printf("Recvd %d bytes, SEQ=%u FL=%u\n", recvd, recvd_seqnum, flags);
        fflush(stdout);

        if(flags & FLAG_FIN)
        {
            printf("Got FIN, going to quit...\n");
            break;
        }
        totPacketsRecvd++;

        /* check packet sequencing */
        if(recvd_seqnum == currentSeqNum)
        {
            /* packet is in correct sequence */
            fwrite(buffer + HEADERSIZE, sizeof(char), recvd - HEADERSIZE, fp);
            currentSeqNum += (recvd - HEADERSIZE);
        } else {
            printf("Got unexpected packet!\n");
        }

        /* send ACK */
        struct tcp_header header = {
            .seqNum = 0,
            .ackNum = currentSeqNum
        };
        createAndSendPacket(s, NULL, 0, &header, (struct sockaddr *) &si_other);
        
        printf("\n");
        
        memset(buffer, 0, PACKETSIZE);
        // usleep(1000);
        recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                         (struct sockaddr *) &si_other, &slen);
    }

    /* send FINACK packet */
    struct tcp_header header = {
        .flags = FLAG_FIN | FLAG_ACK,
    };
    printf("Sending FINACK! ");
    createAndSendPacket(s, NULL, 0, &header, (struct sockaddr *) &si_other);

    clock_gettime(CLOCK_REALTIME, &endspec);
    unsigned int sec = endspec.tv_sec - startspec.tv_sec;
    unsigned int msec = (endspec.tv_nsec - startspec.tv_nsec) / 1000000;
    printf("\nTime taken = %u.%u\n", sec, msec);

    close(s);
    fclose(fp);
    printf("%s received.\n", destinationFile);
}


/*
 *
 */
int main(int argc, char** argv)
{
    unsigned short int udpPort;

    if (argc != 3)
    {
        fprintf(stderr, "usage: %s UDP_port filename_to_write\n\n", argv[0]);
        exit(1);
    }

    udpPort = (unsigned short int) atoi(argv[1]);
    reliablyReceive(udpPort, argv[2]);
    printf("\nPacket stats: Recvd %d Sent %d\n", totPacketsRecvd, totPacketsSent);

    return (EXIT_SUCCESS);
}
