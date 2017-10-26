#include "tcp.h"

void reliablyReceive(unsigned short myUDPport, char* destinationFile)
{
 	FILE *fp;
    fp = fopen(destinationFile, "wb");
    //if (fp == NULL)
        //diep("Could not open file to save.\n");

    TCP tcp(myUDPport);
    thread sendACK(&TCP::receiverSendACK, &tcp);
    thread receiveData(&TCP::receiverReceiveData, &tcp, fp);
    sendACK.join();
    receiveData.join();

    fclose(fp);
   //  printf("%s received.\n", destinationFile);
}
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
   // printf("\nPacket stats: Recvd %d Sent %d\n", totPacketsRecvd, totPacketsSent);

    return (EXIT_SUCCESS);
}
