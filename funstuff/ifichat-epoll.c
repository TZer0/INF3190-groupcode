#include <stdio.h>
#include <unistd.h>

#include <stdlib.h>

#include <memory.h>

#include <netinet/in.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/epoll.h>
#include <assert.h>

#define NUMCLIENTS 10
#define BUFSIZE 1024
#define EPOLLEVENTS 10

struct sockinfo
{
	int fd;
	struct sockaddr_in saddr;
	char buffer[BUFSIZE];
	int bufpos;
};

struct sockinfo clients[NUMCLIENTS];
static int epollfd;


/* Accept a new client and add it to the clients table.
 * Or immediately close the connection if the clients
 * table is full
 */
void accept_client(int infd)
{
	int i;
	struct sockaddr_in c_addr;
	socklen_t addrlen = sizeof(c_addr);

	int newfd = accept(infd, (struct sockaddr*)&c_addr, &addrlen);

	if(newfd < 0)
	{
		perror("accept");
		return;
	}

	/* Find available spot in clients table */
	for(i = 0; i < NUMCLIENTS; i++)
	{
		if(clients[i].fd > 0)
			continue;

		/* Make sure we subscribe to events on the socket */
		struct epoll_event event = { .events = EPOLLIN, .data.u32 = i };
		if(epoll_ctl(epollfd, EPOLL_CTL_ADD, newfd, &event) != 0)
		{
			perror("epoll_ctl");
			printf("Failed to set up epoll for new client, dropping connection!\n");
			close(newfd);
			return;
		}

		clients[i].fd = newfd;
		clients[i].saddr = c_addr;
		clients[i].bufpos = 0;

		printf("Accepted new client with id=%d\n", i);
		return;
	}

	printf("Server filled up! Rejected new client!\n");
	close(newfd);
	return;
}

/* Add a message from client id fromid
 * to the buffers of all other clients
 */
void add_to_bufs(int fromid, const char* stuff, size_t len)
{
	int i;
	struct epoll_event event;

	for(i = 0; i < NUMCLIENTS; ++i)
	{
		if(i == fromid || clients[i].fd < 0)
			continue;

		if(clients[i].bufpos+len > BUFSIZE)
		{
			printf("Buffer full for client %d, dropping client! (pos=%d)\n", i, clients[i].bufpos);
			close(clients[i].fd);
			clients[i].fd = -1;
			continue;
		}

		/* If the buffer was empty we were not subscribed to the
		 * EPOLLOUT event, and we must subscribe to it
		 */
		if(clients[i].bufpos == 0)
		{
			event.events = EPOLLIN | EPOLLOUT;
			event.data.u64 = 0; // Valgrind complains when half the value is uninitialized
			event.data.u32 = i;

			if(epoll_ctl(epollfd, EPOLL_CTL_MOD, clients[i].fd, &event) != 0)
			{
				perror("epoll_ctl");
				printf("Failed to add write watch for client %d, closing socket\n", i);
				close(clients[i].fd);
				clients[i].fd = -1;
				continue;
			}
		}

		printf("Adding %ld bytes to buffer for %d, (from=%d)\n", len, i, fromid);
		memcpy(clients[i].buffer+clients[i].bufpos, stuff, len);
		clients[i].bufpos += len;
	}
}

int main(int argc, char* argv[])
{
	int  retv, i;      /* Random locals */
	char buf[BUFSIZE]; /* Buffer for receiving data */
	int  infd;         /* Listenind socket FD */

	struct epoll_event event;               /* Used for changing epoll events */
	struct epoll_event events[EPOLLEVENTS]; /* Events returned by epoll_wait */

	(void)argc; 
	(void)argv; /* To shut up the compiler */

	infd = socket(PF_INET, SOCK_STREAM, 0);
	if(infd < 0)
	{
		perror("socket");
		exit(-1);
	}

	struct sockaddr_in baddr;
	memset(&baddr, 0, sizeof(baddr));
	baddr.sin_family = PF_INET;
	baddr.sin_addr.s_addr = INADDR_ANY;

	/* Bind until we successfully bind! */
	i = 1445;
	while(i < 1500)
	{
		baddr.sin_port = htons(i);
		retv = bind(infd, (struct sockaddr*)&baddr, sizeof(baddr));

		if(retv < 0)
		{
			perror("bind");
		} else {
			printf("Successfully bound on %d\n", i);
			break;
		}

		++i;
		printf("Trying port %d\n", i);
	}

	/* ... or fail miserably =( */
	if(i == 1500)
	{
		printf("Giving up on binding socket...\n");
		close(infd);
		exit(-2);
	}

	retv = listen(infd, 5);

	if(retv < 0)
	{
		perror("listen");
		close(infd);
		exit(-3);
	}

	for(i = 0; i < NUMCLIENTS; ++i)
	{
		clients[i].fd = -1;
		clients[i].bufpos = 0;
	}

	/* Create the epoll fd, the size argument is ignored */
	epollfd = epoll_create(10);

	event.events = EPOLLIN;        /* For new connections */
	event.data.u64 = 0;            /* Valgrind complaint  */
	event.data.u32 = (uint32_t)-1; /* So we know it's the listening socket, */
	                               /* the other sockets use their index in the clients table. */

	/* Now actually tell epoll to watch infd for the EPOLLIN event. */
	retv = epoll_ctl(epollfd, EPOLL_CTL_ADD, infd, &event);
	if(retv != 0)
	{
		perror("epoll_ctl");
		close(infd);
		close(epollfd);
		exit(-4);
	}


	while(1)
	{
		/* Wait for something to happen.                  */
		/* Up to EPOLLEVENTS amount of events can happen. */
		retv = epoll_wait(epollfd, events, EPOLLEVENTS, -1); 

		if(retv < 0)
		{
			perror("epoll_wait");
			close(infd);
			close(epollfd);
		}

		/* Iterate over the events */
		for(i = 0; i < retv; ++i)
		{
			struct epoll_event *cur = &events[i];

			if(cur->data.u32 == (uint32_t)-1) /* Check if it was the listening socket... */
			{
				accept_client(infd);
			} else {                          /* or client socket */
				assert(cur->data.u32 < NUMCLIENTS);
				struct sockinfo *curclient = &clients[cur->data.u32]; 

				if(cur->events & EPOLLIN) /* Data is available to read */
				{
					ssize_t recvlen = recv(curclient->fd, buf, BUFSIZE, 0);

					if(recvlen < 0)
					{
						perror("recv");
						printf("An error occured during read from client %d\n", cur->data.u32);
						close(curclient->fd);
						curclient->fd = -1;
						continue;
					}

					if(recvlen == 0)
					{
						printf("Client %d closed connection\n", cur->data.u32);
						close(curclient->fd);
						curclient->fd = -1;
						continue;
					}

					printf("Received %ld bytes from client %d\n", recvlen, i);

#ifdef DEBUG
					printf("--Data: %.*s\n", recvlen, buf);
#endif	
					add_to_bufs(cur->data.u32, buf, recvlen);
				}

				if(cur->events & EPOLLOUT) /* We can write to the socket without blocking */
				{
					ssize_t sendlen = send(curclient->fd, curclient->buffer, curclient->bufpos, 0);

					if(sendlen <= 0)
					{
						printf("Lost connection to client %d\n", cur->data.u32);
						close(curclient->fd);
						curclient->fd = -1;
						continue;
					}

					if(sendlen < curclient->bufpos)
					{
						memmove(curclient->buffer, curclient->buffer+sendlen, curclient->bufpos-sendlen);
					}
					else
					{
						event.events = EPOLLIN; /* No longer watch for EPOLLOUT */
						event.data.u32 = cur->data.u32;

						if(epoll_ctl(epollfd, EPOLL_CTL_MOD, curclient->fd, &event) != 0)
						{
							perror("epoll_ctl");
							printf("Dropping connection to client %d\n", cur->data.u32);
							close(curclient->fd);
							curclient->fd = -1;
						}
					}

					curclient->bufpos -= sendlen;

					printf("Sent %ld bytes to client %i (new bufpos=%d)\n", sendlen, cur->data.u32, curclient->bufpos);
				}
			}
		}
	}
}
