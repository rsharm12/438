#include "tcp.h"

using namespace std;


void reliablyTransfer(string hostname, uint16_t udpPort,
                      string filename, uint64_t bytesToTransfer)
{
    using namespace std::chrono;

    TCP::UDP udp(udpPort, hostname);

    ifstream infile(filename, ios::binary);
    if (!infile.is_open())
        TCP::diep("Could not open file to send");

    cout << "Need to send " << bytesToTransfer << " bytes to ";
    cout << hostname << ":" << udpPort << endl;

    TCP::starttime = high_resolution_clock::now();

    TCP::CongestionControl cc(bytesToTransfer);

    thread sendData(&TCP::Sender::sendData, &udp, &cc);
    thread recvACK(&TCP::Sender::recvACK, &udp, &cc);
    thread updateWindow(&TCP::Sender::updateWindow, &cc, &infile);
    sendData.join();
    recvACK.join();
    updateWindow.join();

    int recvd;
    char buffer[PACKETSIZE];
    fd_set set;
    struct timeval timeout;

    /* send FIN packet */
    while(1)
    {
        TCP::Packet packet(TCP::Header(0, 0, FLAG_FIN, 0), nullptr, 0);

        packet.toBuffer(buffer);

        /* send FIN */
        cout << "Sending FIN...";
        packet.header.log();
        udp.send(buffer, HEADERSIZE);

        FD_ZERO(&set);
        FD_SET(udp.sock, &set);

        /* check for FINACK */
        /* reset timeout to 1 sec for fin */
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        select(udp.sock+1, &set, NULL, NULL, &timeout);

        memset(buffer, 0, PACKETSIZE);

        if(FD_ISSET(udp.sock, &set))
        {
            recvd = udp.recv(buffer, PACKETSIZE);

            /* extract FLAGS from received packet */
            TCP::Header gotHeader = TCP::Packet::extractHeader(buffer);
            gotHeader.log();

            cout << "Got " << recvd << " bytes.";
            gotHeader.log();

            if(gotHeader.flags & (FLAG_FIN | FLAG_ACK))
            {
                cout << "Got FINACK!" << endl;
                break;
            }
        } else {
            /* timeout */
            cout << "No FINACK received..." << endl;
        }
    }

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
