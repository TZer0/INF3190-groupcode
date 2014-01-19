#include<stdio.h>
#include<sys/socket.h>
#include<netdb.h>
#include<time.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>

// Merge with client

int main (int args, char **argv) {
	char *port;
	if (args != 2) {
		fprintf(stderr,"Usage: %s [port_number]\nWarning, using default port: 6789.\n", argv[0]);
		port = "6789";
	} else {
		port = argv[1];
	}

	// Fix this!
	char buf[101];
	buf[0] = '\0';
	int request_sd, sd;
	int i, rc, connected = 0;
	socklen_t clientaddrlen;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	struct timeval timeout;
	fd_set fds, fdscopy;
	timeout.tv_sec = 999999;
	timeout.tv_usec = 0;

	if ((request_sd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))< 0) {
		printf("Failed to take socket, aborting!\n");
		return -1;
	}
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));
	memset(&clientaddr, 0, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(atoi(port));
	if ((bind(request_sd, (struct sockaddr*) &serveraddr, sizeof(struct sockaddr_in))) < 0) {
		printf("Failed to bind, aborting!\n");
		return -1;
	}
	if ((listen(request_sd, SOMAXCONN)) < 0) {
		printf("Failed to listen, aborting!\n");
		return -1;
	}
	FD_ZERO(&fds);
	FD_SET(request_sd, &fds);
	FD_SET(STDIN_FILENO, &fds);
	fprintf(stderr, "Listening for connections on port '%s'\n", port);
	while (1) {
		fdscopy = fds;
		// Fix this
		rc = select(FD_SETSIZE, &fdscopy, NULL, NULL, &timeout);
		if (rc < 0) {
			break;
		} else if (rc == 0) {
			fprintf(stderr,"timeout!");
			timeout.tv_sec = 999999;
		} else {
			if (FD_ISSET(STDIN_FILENO, &fdscopy)) {
				fgets(buf, 100, stdin);
				if  (connected) {
					send(connected, buf, strlen(buf)+1, 0);
				}

			}
			for (i = request_sd; i < 2+request_sd; i++) {
				if (FD_ISSET(i, &fdscopy)) {
					if (i == request_sd) {
						clientaddrlen = sizeof(struct sockaddr_in);
						// Display client information
						sd = accept(request_sd, (struct sockaddr*) &clientaddr, &clientaddrlen);
						FD_SET(sd, &fds);
						connected = sd;
					} else {
						if (0 < recv(i, buf, 100, 0)) {
							printf("%s", buf);
						}
					}
				}
			}
		}
	}
}
