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
struct sockaddr_storage si_other;
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
    recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                     (struct sockaddr *) &si_other, &slen);

    /* set to non-blocking socket after 1st receive */
    opts = opts | O_NONBLOCK;
    fcntl(s, F_SETFL, opts);

    while(recvd > 0)
    {
        totPacketsRecvd++;

        /* copy header to new buffer */
        // memset(header, 0, HEADERSIZE);
        // memcpy(header, buffer, HEADERSIZE);
        // sscanf(header, "%u%u", &seqnum, &acknum);
        recvd_seqnum = *((unsigned int *) &buffer[0]);
        recvd_acknum = *((unsigned int *) &buffer[4]);
        printf("Recvd %d bytes, SEQ=%u ACK=%u", recvd, recvd_seqnum, recvd_acknum);

        /* check packet sequencing */
        if(recvd_seqnum == currentSeqNum)
        {
            /* packet is in correct sequence */
            fwrite(buffer + HEADERSIZE, sizeof(char), recvd - HEADERSIZE, fp);
            /* send ACK */
        } else {
            /* packet not expected, send ACK for last correct packet */
            
            sendto(s, packet, HEADERSIZE + bufSize, 0,
                   (struct sockaddr *) &si_other, slen);
        }

        memset(buffer, 0, PACKETSIZE);
        usleep(1000);
        recvd = recvfrom(s, buffer, PACKETSIZE, 0,
                         (struct sockaddr *) &si_other, &slen);
    }

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
