#include<stdio.h>
#include<sys/socket.h>
#include<netdb.h>
#include<time.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>

int main (int args, char **argv) {
	char *port, *host;
	int error = 0;
	if (args < 3) {
		fprintf(stderr,"Warning, using default port: 6789.\n");
		port = "6789";
		error = 1;
	} else {
		port = argv[2];
	}

	if (args < 2) {
		fprintf(stderr, "Warning: no host given, using localhost.\n");
		host = "localhost";
		error = 1;
	} else {
		host = argv[1];
	}
	if (error) {
		fprintf(stderr,"Usage: %s [host] [port_number]\n", argv[0]);
	}

	char buf[101];
	buf[0] = '\0';
	int sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	int rc;
	socklen_t clientaddrlen;
	struct sockaddr_in serveraddr;
	struct timeval timeout;
	fd_set fds, fdscopy;
	timeout.tv_sec = 999999;
	timeout.tv_usec = 0;

	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	struct hostent *he = gethostbyname(host);
	memcpy(&serveraddr.sin_addr, he->h_addr, sizeof(struct in_addr));
	serveraddr.sin_port = htons(atoi(port));
	if (connect (sd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in) ) > 0) {
		fprintf(stderr, "Connect error");
		return -1;
	}
	FD_ZERO(&fds);
	FD_SET(sd, &fds);
	FD_SET(STDIN_FILENO, &fds);
	fprintf(stderr, "Chatting with '%s' on port %s\n", host,  port);
	while (1) {
		fdscopy = fds;
		rc = select(FD_SETSIZE, &fdscopy, NULL, NULL, &timeout);
		if (rc < 0) {
			break;
		} else if (rc == 0) {
			fprintf(stderr,"timeout!");
			timeout.tv_sec = 999999;
		} else {
			if (FD_ISSET(STDIN_FILENO, &fdscopy)) {
				fgets(buf, 100, stdin);
				send(sd, buf, strlen(buf)+1, 0);

			}
			if (FD_ISSET(sd, &fdscopy)) {
				if (0 < recv(sd, buf, 100, 0)) {
					printf("%s", buf);
				}
			}
		}
	}
}
