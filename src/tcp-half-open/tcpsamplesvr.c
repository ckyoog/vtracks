#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CALL(ret, bad, msg) \
	({\
	 	int i;\
		if ((i = (ret)) == (bad)) {\
			perror(msg);\
			exit(1);\
		}\
		i;\
	})
#define CALL1(funcname, a...)	({\
 	int ret;\
	if ((ret = funcname( a )) == -1) {\
		perror(#funcname);\
		exit(1);\
	}\
	ret;\
})
#define SCALL(ret, msg) CALL(ret, -1, msg)

void connection_handler(int connfd);

int main()
{
	pid_t pid;
	int len, connfd;
	int sockfd = SCALL(socket(AF_INET, SOCK_STREAM, 0), "socket");
	struct sockaddr_in addr;
	struct sockaddr *paddr = (struct sockaddr *)&addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(12345);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	SCALL(bind(sockfd, paddr, sizeof(addr)), "bind");
	SCALL(listen(sockfd, 5), "listen");

	while ((connfd = accept(sockfd, paddr, &len)) != -1) {
		printf("connected %d\n", connfd);
		pid = fork();

		if (pid == 0) {		/* subprocess */
			close(sockfd);
			connection_handler(connfd);
			close(connfd);
			printf("disconnected %d\n", connfd);
			exit(0);
		} else if (pid == -1) {
			perror("fork");
			exit(1);
		}

		close(connfd);
	}

	if (connfd == -1) perror("accept");
	close(sockfd);
	return 0;
}

void connection_handler(int connfd)
{
	char *hello = "hello";
	char buf[1024];
	int n, ret;
	struct timeval to;
	fd_set rfds;

	to.tv_sec = 5;
	to.tv_usec = 0;

	FD_ZERO(&rfds);
	FD_SET(connfd, &rfds);

	while (ret = select(connfd+1, &rfds, NULL, NULL, &to)) {
		if (ret == -1) {
			perror("select");
			exit(1);
		} else if (ret == 0) {
			/* write to client */
			write(connfd, hello, strlen(hello)+1);
		} else {
			if (FD_ISSET(connfd, &rfds)) {
				n = recv(connfd, buf, sizeof(buf), 0);
				if (n == 0)	/* client disconnected */
					return;
				printf("recv %d bytes, '%s'\n", n, buf);
			}
		}
		to.tv_sec = 5;
		to.tv_usec = 0;
	}
}

