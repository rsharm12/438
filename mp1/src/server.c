/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define BACKLOG 10	 // how many pending connections queue will hold

#define GET_REQ_LEN 4
#define MAXBUFLEN 1024
#define MAXFILELEN 1024

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
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
	int sockfd, new_fd, port, numbytes, numfnbytes, numfbytes;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1, i;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char filename[MAXFILELEN];
	char buf[MAXBUFLEN];
	char resbuf[MAXBUFLEN];
	char GET[GET_REQ_LEN];
	char *vHTTPstart;
	int len200, len400, len404;
	FILE *fp;


	const char *HTTP_RESP[3];
	HTTP_RESP[0] = "HTTP/1.1 200 OK\r\n";
	HTTP_RESP[1] = "HTTP/1.1 400 Bad Request\r\n";
	HTTP_RESP[2] = "HTTP/1.1 404 Not Found\r\n";
	len200 = strlen(HTTP_RESP[0]);
	len400 = strlen(HTTP_RESP[1]);
	len404 = strlen(HTTP_RESP[2]);

	if (argc != 2) {
	    fprintf(stderr,"usage: server port\n");
	    exit(1);
	}
	
	bzero(resbuf, MAXBUFLEN);
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);

	//	vHTTPstart = strstr(buf, "HTTP");
	//	strncat(HTTP_RESP[1], vHTTPstart, );

		if (!fork()) { // this is the child process
			close(sockfd); // child doesn't need the listener

			numbytes = recv(new_fd, buf, MAXBUFLEN-1 , 0);
			if (numbytes == -1) {
				perror("recv");
				exit(1);
			}
	
			buf[numbytes]='\0';
			printf("received %d bytes from client", numbytes);
	
			memcpy(GET, buf, 3);
			GET[GET_REQ_LEN]='\0';

			// HTTP 400 Bad Request (i.e. not GET)
			if(strcmp(GET, "GET\0") != 0) {
				memcpy(resbuf, HTTP_RESP[1], len400);
				numfbytes=len400;
				if (write(new_fd, resbuf, numfbytes) == -1)
					perror("write");
				close(new_fd);
				exit(0);
			}

			// extract filename
			i=4; // start after "GET "
			while(buf[i] != '\r')
			{
				numfnbytes++;
				i++;
			}

			bzero(filename, MAXFILELEN);
			memcpy(filename, buf+4, numfnbytes-9);
			fprintf(stderr, "filename: %s\n", filename);

			//GET abc HTTP/1.1\r\n
			fp = fopen(filename, "r");

			// HTTP 200 OK
			if( fp != NULL ) {
				memcpy(resbuf, HTTP_RESP[0], len200);
				if( (numfbytes = fread(resbuf+len200, sizeof(char), 
							MAXBUFLEN, fp)) == 0 ) {
					memcpy(resbuf+len200, "File is empty!\0", 15);
				numfbytes = 15;
				}
			}
			// HTTP 404 Not Found
			else {
				memcpy(resbuf, HTTP_RESP[2], len404);
				numfbytes=len404;
				exit(1);
			}

			// write HTTP response to client
			if (write(new_fd, resbuf, numfbytes) == -1)
					perror("write");
			close(new_fd);
			exit(0);
		}
		close(new_fd);  // parent doesn't need this
	}

	return 0;
}

