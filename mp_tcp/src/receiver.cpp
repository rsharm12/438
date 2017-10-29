#include "tcp.h"

using namespace std;


void reliablyReceive(uint16_t udpPort, string filename)
{
    TCP::UDP udp(udpPort);

    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open())
        TCP::diep("Could not open file to send");

    mutex rcvrACK_mtx;
    queue<uint32_t> rcvrACK_q;

    thread receiveData(&TCP::Receiver::receiveData, &udp, &outfile, &rcvrACK_mtx, &rcvrACK_q);
    thread sendACK(&TCP::Receiver::sendACK, &udp, &rcvrACK_mtx, &rcvrACK_q);
    receiveData.join();
    sendACK.join();
}

int main(int argc, char** argv)
{
    uint16_t udpPort;

    if (argc != 3)
    {
        cerr << "usage: " << argv[0] << "UDP_port filename_to_write";
        cerr << endl;
        exit(1);
    }

    udpPort = static_cast<uint16_t>(stoul(argv[2]));

    reliablyReceive(udpPort, argv[3]);

    return (EXIT_SUCCESS);
}
