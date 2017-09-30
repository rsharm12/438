/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define MAXSTRSIZE  100
#define PORTLEN     6
#define BUFSIZE     1024 // max number of bytes we can get at once

char getrequest[] = "GET %s HTTP/1.1\r\nHost: %s:%s\r\nConnection: Keep-Alive\r\n\r\n";


// parse input url for hostname and port number
void parseURL(char *url, char *hostname, char *port, char *filepath)
{
	char *hostStart, *portStart, *pathStart, *end;

	hostStart = strstr(url, "://");
	if(!hostStart) {
		// user did not specify the protocol
		hostStart = url;
	} else {
		hostStart += 3;
	}

	portStart = strstr(hostStart, ":");
	if(portStart)
		portStart++;

	pathStart = hostStart;
	while(*pathStart != '\0' && *pathStart != '/')
		pathStart++;

	end = pathStart;
	while(*end != '\0')
		end++;

	if(portStart) {
		strncpy(hostname, hostStart, (size_t)(portStart - hostStart) - 1);
		strncpy(port, portStart, (size_t)(pathStart - portStart));
	} else {
		strncpy(hostname, hostStart, (size_t)(pathStart - hostStart));
		strcpy(port, "80");
	}
	if(pathStart == end)
		strcpy(filepath, "/");
	else
		strncpy(filepath, pathStart, (size_t)(end - pathStart));
}


void send_request(int sockfd, const char * msg, int len)
{
    int bytes_sent, ret;

    bytes_sent = 0;
    while (bytes_sent < len)
    {
        ret = send(sockfd, msg + bytes_sent, len, 0);
        if (ret == -1)
        {
            perror("http_client: send request");
            close(sockfd);
            exit(3);
        }

        bytes_sent += ret;
    }
}


/*
 * Tries to receive the response for a HTTP request.
 * Returns:
 *   0 - success
 *   1 - 404 Not Found
 *  -1 - error
 */
int recv_data(int sockfd, char *buf)
{
    int ret, total, idx, found;
    struct timeval tv;

    tv.tv_sec = 3;   // 5 Secs Timeout
    tv.tv_usec = 0;  // 0 extra microseconds

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));

    // open the output file
    FILE * outfile = fopen("output", "w");
    if (outfile == NULL)
    {
        fprintf(stderr, "http_client: failed to open output file\n");
        return -1;
    }

    found = 0;
    total = 0;

    // check response code
    memset(buf, 0, BUFSIZE);
    ret = recv(sockfd, buf, BUFSIZE, 0);
    if (ret == -1)
    {
        perror("client: recv message");
        fclose(outfile);
        return -1;
    }

    // compare the response
    if (0 == strncmp(buf + 9, "404", 3))
    {
        printf("HTTP Response: 404 Not Found\n");
        fclose(outfile);
        return 1;
    }
    else if (0 == strncmp(buf + 9, "200", 3))
    {
        // check for start of message body
        while (!found)
        {
            if (ret == 0)
            {
                // got no more data, can't do anything now
                fclose(outfile);
                return -1;
            }

            // scan through the received data
            idx = 0;
            for (idx = 0; idx < ret; idx++)
            {
                if (buf[idx] == '\r')
                {
                    if (0 == strncmp(buf+idx, "\r\n\r\n", 4))
                    {
                        found = 1;
                        idx += 4;
                        break;
                    }
                }
            }

            // print the header
            printf("%.*s", idx, buf);

            // receive more data
            if (!found)
            {
                memset(buf, 0, BUFSIZE);
                ret = recv(sockfd, buf, BUFSIZE, 0);
                if (ret == -1)
                {
                    perror("client: recv message");
                    fclose(outfile);
                    return -1;
                }
            }
        }

        // write the rest of the data that was already received
        fwrite(buf + idx, 1, ret - idx, outfile);
        total += ret - idx;

        while (ret > 0)
        {
            memset(buf, 0, BUFSIZE);
            ret = recv(sockfd, buf, BUFSIZE, 0);
            if (ret == -1)
            {
                perror("client: recv message");
                fclose(outfile);
                return -1;
                break;
            }

            total += ret;
            fwrite(buf, 1, ret, outfile);
        }
        printf("Done writing %d bytes of data\n", total);
        fclose(outfile);

        return 0;
    }
    else
    {
        printf("Unknown response code received: %.3s\n", buf + 9);
        printf("%s\n", buf);
        fclose(outfile);
        return -1;
    }
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int main(int argc, char *argv[])
{
	int sockfd, numbytes;  
	char buf[BUFSIZE];

	char hostname[MAXSTRSIZE];
	char port[PORTLEN];
	char filepath[MAXSTRSIZE];

	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: http_client http://hostname[:port]/path/to/file\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	memset(hostname, 0, MAXSTRSIZE);
	memset(port, 0, PORTLEN);
	memset(filepath, 0, MAXSTRSIZE);
	
	parseURL(argv[1], hostname, port, filepath);
	printf("http_client: hostname %s\n", hostname);
	printf("http_client: port %s\n", port);
	printf("http_client: filepath %s\n", filepath);

	if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("http_client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("http_client: connect");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "http_client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("http_client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	memset(buf, 0, BUFSIZE);
	// send "GET" request
	sprintf(buf, getrequest, filepath, hostname, port);
	numbytes = strlen(buf);
	send_request(sockfd, buf, numbytes);

	if(0 != recv_data(sockfd, buf))
	{
		fprintf(stderr, "http_client: could not complete request\n");
	}

	close(sockfd);
	return 0;
}