#include <stdio.h>
#include <ctype.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "../common.h"
#include "handle_request.h"


void *serveClient(void *arg);

int main(int argc, char **argv){
	if (argc > 1){
		if (!strcmp(argv[1], "--debug"))
			stream = stderr;
		else{
			printf("unkown option: %s\n", argv[1]);
			exit(1);
		}
	}else
		stream = fopen("/dev/null", "w");

	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		perror("socket");
		return 1;
	}

	int on = 1;
	setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));
	servsin.sin_addr.s_addr = htonl(INADDR_ANY);
	servsin.sin_family = AF_INET;
	servsin.sin_port = htons(6969);

	if ((bind(servsock, (struct sockaddr*) &servsin, sizeof(servsin))) < 0){
		perror("bind");
		return 1;
	}

	if ((listen(servsock, 10)) < 0){
		perror("listen");
		return 1;
	}

	struct net_info *cli_info = malloc(sizeof(struct net_info));
	cli_info->data_port = 0;
	struct sockaddr_in clisin;
	unsigned int sin_len = sizeof(clisin);
	bzero(&clisin, sizeof(clisin));

	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	/* serve multiple clients simultaneously */
	while (1){
		cli_info->sockfd = accept(servsock, (struct sockaddr*) &clisin, &sin_len);
		if (cli_info->sockfd < 0){
			perror("accept connection");
			continue;
		}

		inet_ntop(AF_INET, &(clisin.sin_addr), cli_info->ip_add, sizeof(cli_info->ip_add));
		cli_info->port = ntohs(clisin.sin_port);

		/* create a new thread for each client */
		pthread_t tid;
		int thr = pthread_create(&tid, NULL, &serveClient, (void*)cli_info);
		if (thr != 0){
			char err_mess[255];
			strerror_r(errno, err_mess, sizeof(err_mess));
			fprintf(stream, "create thread to handle %s:%u: %s\n", 
					cli_info->ip_add, cli_info->port, err_mess);
			close(cli_info->sockfd);
			continue;
		}
		cli_info = malloc(sizeof(struct net_info));
		cli_info->data_port = 0;
	}
	close(servsock);
}

void *serveClient(void *arg){
	return NULL;
}
