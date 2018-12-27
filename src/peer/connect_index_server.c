#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>

#include "../common.h"
#include "../sockio.h"
#include "connect_index_server.h"


int connect_to_index_server(){
	/* connect to index server */
	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));

	servsin.sin_family = AF_INET;

	char *servip;
	char buf[22];
	printf("Enter server's IP addr and port in the form of <IP>:<port>\n");
	scanf("%s", buf);
	getchar();	/* remove '\n' character from the stdin */

	/* parse the buf to get ip address and port */
	servip = strtok(buf, ":");
	servsin.sin_addr.s_addr = inet_addr(servip);
	servsin.sin_port = htons(atoi(strtok(NULL, ":")));

	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		print_error("socket");
		exit(2);
	}

	if (connect(servsock, (struct sockaddr*) &servsin, sizeof(servsin)) < 0){
		print_error("connect");
		exit(3);
	}

	return servsock;
}
