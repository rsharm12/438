#include "tcp.h"

using namespace std;


void reliablyTransfer(string hostname, uint16_t udpPort,
                      string filename, uint64_t bytesToTransfer)
{
    using namespace std::chrono;

    ifstream infile(filename, ios::binary);
    if (!infile.is_open())
        TCP::diep("Could not open file to send");

    cout << "Need to send " << bytesToTransfer << " bytes to ";
    cout << hostname << ":" << udpPort << endl;

    TCP::UDP udp(udpPort, hostname);
    TCP::CongestionControl cc(bytesToTransfer);
    TCP::Sender::senderTimeouts = 0;

    // TCP::Sender::setupConnection(&udp);
    // TCP::starttime = high_resolution_clock::now();

    thread sendData(&TCP::Sender::sendData, &udp, &cc);
    thread recvACK(&TCP::Sender::recvACK, &udp, &cc);
    thread updateWindow(&TCP::Sender::updateWindow, &cc, &infile);
    sendData.join();
    recvACK.join();
    updateWindow.join();

    TCP::endtime = high_resolution_clock::now();
    duration<double> time_span =
        duration_cast<duration<double>>(TCP::endtime - TCP::starttime);

    cout << "Took " << time_span.count() << " seconds" << endl;
    cout << "Packet Stats: Sent=" << TCP::packetsSent;
    cout << " Recvd=" << TCP::packetsRecvd;
    cout << " Timeouts=" << TCP::Sender::senderTimeouts << endl;
    // cout << "min packets=" << bytesToTransfer/DATASIZE << endl;

    /* send FIN packet */
    TCP::Sender::closeConnection(&udp);
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
