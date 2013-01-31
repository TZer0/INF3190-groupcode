#include<stdio.h>
#include<sys/socket.h>
#include<netdb.h>
#include<time.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>

// Merge with client

int main (int args, char **argv) {
	char *port, *host;
	int server = 0;
	int i, rc, connected = 0, sd = 0, len;
	struct sockaddr_in clientaddr, serveraddr;
	struct timeval timeout;
	// Fix this!
	char ack = 1, pkg = 0, recvpkg = 0, sbuf[102], rbuf[102];
	fd_set fds, fdscopy;

	if (args != 2 && args != 3) {
		printf("Server: %s [port_number]\n", argv[0]);
		printf("Client: %s [host] [port_number]\n", argv[0]);
		return 0;
	} else if(args == 1) {
		printf("Warning, using default port: 6789.\n");
		port = "6789";
	}

	if (args <= 2) {
		printf("Launching server.\n");
		if (args == 2) {
			port = argv[1];
		}
		server = 1;
	} else if(args == 3) {
		printf("Launching client.\n");
		port = argv[2];
		host = argv[1];
	}

	socklen_t addrlen = sizeof(struct sockaddr_in);

	FD_ZERO(&fds);
	memset(&serveraddr, 0, sizeof(struct sockaddr_in));

	if (server)  {
		if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))< 0) {
			printf("Failed to take socket, aborting!\n");
			return -1;
		}

		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = INADDR_ANY;
		serveraddr.sin_port = htons(atoi(port));
		if ((bind(sd, (struct sockaddr*) &serveraddr, sizeof(struct sockaddr_in))) < 0) {
			printf("Failed to bind, aborting!\n");
			return -1;
		}

		fprintf(stderr, "Listening for connections on port '%s'\n", port);
	} else {
		// This is a client.
		sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		memset(&serveraddr, 0, sizeof(struct sockaddr_in));
		serveraddr.sin_family = AF_INET;
		struct hostent *he = gethostbyname(host);
		memcpy(&serveraddr.sin_addr, he->h_addr, sizeof(struct in_addr));
		serveraddr.sin_port = htons(atoi(port));
		if (connect (sd, (struct sockaddr *) &serveraddr, sizeof(struct sockaddr_in) ) > 0) {
			fprintf(stderr, "Connect error\n");
			return -1;

		}
	}

	FD_SET(sd, &fds);
	FD_SET(STDIN_FILENO, &fds);

	memset(&clientaddr, 0, sizeof(struct sockaddr_in));
	while (1) {
		fdscopy = fds;
		// Fix this
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;
		rc = select(FD_SETSIZE, &fdscopy, NULL, NULL, &timeout);
		if (rc < 0) {
			perror("Select error");
			break;
		} else if (rc == 0) {
			if (ack == 0) {
				sendto(sd, sbuf, strlen(&sbuf[1])+2, 0,(struct sockaddr *)&serveraddr, addrlen );
			}
		} else {
			if (ack == 1 && FD_ISSET(STDIN_FILENO, &fdscopy)) {
				fgets(&sbuf[1], 100, stdin);
				pkg++;
				sbuf[0] = pkg;
				printf("Sending: %s\n", &sbuf[1]);
				sendto(sd, sbuf, strlen(&sbuf[1])+2, 0,(struct sockaddr *)&serveraddr, addrlen );
				ack = 0;

			}

			if (!connected && server) {
				accept(sd, (struct sockaddr*) &clientaddr, &addrlen);
				FD_SET(sd, &fds);
				printf("Connected.");
				connected = 1;
			} 

			if (FD_ISSET(sd, &fdscopy)) {
				if (0 < (len = recvfrom(sd, &rbuf, 100, 0, (struct sockaddr *)&serveraddr, &addrlen))) {
					if (len == 1) {
						ack = 1;
					} else { 
						if (rbuf[0] != recvpkg) {
							recvpkg = rbuf[0];
							printf("Received: %s", &rbuf[1]); 
						}
						sendto(sd, &recvpkg, 1, 0,(struct sockaddr *)&serveraddr, addrlen );

					}
				}

			}
		}
	}
}
