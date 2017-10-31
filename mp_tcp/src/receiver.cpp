#include "tcp.h"

using namespace std;


void reliablyReceive(uint16_t udpPort, string filename)
{
    using namespace std::chrono;

    TCP::UDP udp(udpPort);

    ofstream outfile(filename, ios::binary);
    if (!outfile.is_open())
        TCP::diep("Could not open file to send");

    mutex rcvrACK_mtx;
    queue<uint32_t> rcvrACK_q;

    TCP::Receiver::waitForConnection(&udp);

    TCP::starttime = high_resolution_clock::now();

    thread receiveData(&TCP::Receiver::receiveData, &udp, &outfile, &rcvrACK_mtx, &rcvrACK_q);
    thread sendACK(&TCP::Receiver::sendACK, &udp, &rcvrACK_mtx, &rcvrACK_q);
    receiveData.join();
    sendACK.join();

    // TODO: fix duration calculation
    TCP::endtime = high_resolution_clock::now();
    duration<double> time_span =
        duration_cast<duration<double>>(TCP::endtime - TCP::starttime);

    cout << "Took " << time_span.count() << " seconds" << endl;
    cout << "Packet Stats: Sent=" << TCP::packetsSent;
    cout << " Recvd=" << TCP::packetsRecvd << endl;
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

    udpPort = static_cast<uint16_t>(stoul(argv[1]));

    reliablyReceive(udpPort, argv[2]);

    return (EXIT_SUCCESS);
}
