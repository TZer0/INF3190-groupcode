#include<stdio.h>
#include<sys/socket.h>
#include<netdb.h>
#include<time.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>

// Merged with client!

int main (int args, char **argv) {
	char *port, *host;
	int server = 0;
	int i, rc, connected = 0, sd = 0, len;
	struct sockaddr_in addr;
	struct timeval timeout;
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
	memset(&addr, 0, sizeof(struct sockaddr_in));

	if ((sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))< 0) {
		printf("Failed to take socket, aborting!\n");
		return -1;
	}

	if (server)  {
		// This is a server, bind a port.
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(atoi(port));
		if ((bind(sd, (struct sockaddr*) &addr, sizeof(struct sockaddr_in))) < 0) {
			printf("Failed to bind, aborting!\n");
			return -1;
		}

		fprintf(stderr, "Listening for connections on port '%s'\n", port);
	} else {
		// This is a client, connect to the server
		addr.sin_family = AF_INET;
		struct hostent *he = gethostbyname(host);
		memcpy(&addr.sin_addr, he->h_addr, sizeof(struct in_addr));
		addr.sin_port = htons(atoi(port));
		if (connect (sd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in) ) > 0) {
			fprintf(stderr, "Connect error\n");
			return -1;

		}
	}

	FD_SET(sd, &fds);
	FD_SET(STDIN_FILENO, &fds);

	while (1) {
		fdscopy = fds;
		timeout.tv_sec = 0;
		timeout.tv_usec = 1;
		// Reasonable value for select.
		rc = select(sd+1, &fdscopy, NULL, NULL, &timeout);
		if (rc < 0) {
			perror("Select error");
			break;
		} else if (rc == 0) {
			// If we havn't received an ACK, we spam the other end with the message.
			if (ack == 0) {
				sendto(sd, sbuf, strlen(&sbuf[1])+2, 0,(struct sockaddr *)&addr, addrlen );
			}
		} else {
			if (ack == 1 && FD_ISSET(STDIN_FILENO, &fdscopy)) {
				fgets(&sbuf[1], 100, stdin);
				pkg++;
				sbuf[0] = pkg;
				printf("Sending: %s\n", &sbuf[1]);
				sendto(sd, sbuf, strlen(&sbuf[1])+2, 0,(struct sockaddr *)&addr, addrlen );
				// we now await an ACK from the other side.
				ack = 0;

			}

			if (!connected && server) {
				// We re-use addr if on the server.
				accept(sd, (struct sockaddr*) &addr, &addrlen);
				FD_SET(sd, &fds);
				printf("Connected.\n");
				connected = 1;
			} 

			if (FD_ISSET(sd, &fdscopy)) {
				if (0 < (len = recvfrom(sd, &rbuf, 100, 0, (struct sockaddr *)&addr, &addrlen))) {
					if (len == 1) {
						if (rbuf[0] == pkg) {
							// Ack received
							ack = 1;
						}
					} else { 
						if (rbuf[0] != recvpkg) {
							recvpkg = rbuf[0];
#ifdef DEBUG
							printf("Received %d: %s", rbuf[0], &rbuf[1]); 
#else
							printf("Received: %s", &rbuf[1]); 
#endif
						} else {
#ifdef DEBUG
							printf("dupe %d\n", rbuf[0]);
#endif
						}
						// We ACK the message
						sendto(sd, &recvpkg, 1, 0,(struct sockaddr *)&addr, addrlen );

					}
				}

			}
		}
	}
}
