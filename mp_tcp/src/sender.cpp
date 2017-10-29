#include "tcp.h"

using namespace std;


void reliablyTransfer(string hostname, uint16_t udpPort,
                      string filename, uint64_t bytesToTransfer)
{
    ifstream infile(filename, ios::binary | ios::ate);
    if (!infile.is_open())
        TCP::diep("Could not open file to send");

    /* Determine how many bytes to transfer */
    uint64_t fileSize = infile.tellg();
    infile.seekg(0);
    if (bytesToTransfer > fileSize)
        bytesToTransfer = fileSize;

    cout << "Need to send " << fileSize << " bytes to ";
    cout << hostname << ":" << udpPort << endl;

    TCP::Sender tcpSender(udpPort);
    tcpSender.sendData(infile, bytesToTransfer);
}

int main(int argc, char** argv)
{
    uint16_t udpPort;
    uint64_t numBytes;

    if (argc != 5)
    {
        cerr << "usage: " << argv[0] << " receiver_hostname receiver_port";
        cerr << " filename_to_xfer bytes_to_xfer";
        cerr << endl;

        exit(1);
    }
    udpPort = static_cast<uint16_t>(stoul(argv[2]));
    numBytes = atoll(argv[4]);

    reliablyTransfer(argv[1], udpPort, argv[3], numBytes);

    return (EXIT_SUCCESS);
}
