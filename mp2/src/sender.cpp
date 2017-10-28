#include "tcp.h"


using namespace std;

void reliablyTransfer(char* hostname, unsigned short int hostUDPport,
                      char* filename, unsigned long long int bytesToTransfer)
{
 	FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
        perror("Could not open file to send.\n");

    /* Determine how many bytes to transfer */
    fseek(fp, 0L, SEEK_END);
    unsigned long long int fileSize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if(bytesToTransfer > fileSize)
        bytesToTransfer = fileSize;
    printf("Need to send %lld bytes to %s...\n", bytesToTransfer, hostname);

    TCP tcp(hostUDPport);
    tcp.setupSockaddrOther(hostname);
    thread sendData(&TCP::senderSendData, &tcp, fp, bytesToTransfer);
    //thread receiveACK(&TCP::senderReceiveACK, &tcp);
    sendData.join();
    //receiveACK.join();

    fclose(fp);
}


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
    //printf("\nPacket stats: Recvd %d Sent %d\n", totPacketsRecvd, totPacketsSent);

    return (EXIT_SUCCESS);
}
