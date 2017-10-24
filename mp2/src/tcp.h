#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

void diep(char *s)
{
    perror(s);
    exit(1);
}

int createAndBindSocket(unsigned int udpPort, struct sockaddr_in * si_me)
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1)
        diep("socket");

    printf("Now binding to port %d\n", udpPort);
    if (bind(sock, (struct sockaddr*) si_me, sizeof (*si_me)) == -1)
        diep("bind");

    return sock;
}

void setupSockaddrOther(struct sockaddr_in * si_other, const char * hostname,
                        unsigned int udpPort)
{
    /* set up other */
    memset((char *) si_other, 0, sizeof (*si_other));
    si_other->sin_family = AF_INET;
    si_other->sin_port = htons(udpPort);
    if (inet_pton(AF_INET, hostname, &(si_other->sin_addr)) != 1)
        diep("inet_pton() failed\n");
}

void setupSockaddrMe(struct sockaddr_in * si_me, unsigned int udpPort)
{
    /* set up me */
    memset((char *) si_me, 0, sizeof (*si_me));
    si_me->sin_family = AF_INET;
    si_me->sin_port = htons(udpPort);
    si_me->sin_addr.s_addr = htonl(INADDR_ANY);
}