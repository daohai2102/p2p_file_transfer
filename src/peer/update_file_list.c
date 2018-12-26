#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "update_file_list.h"
#include "../common.h"
#include "../sockio.h"


uint16_t dataPort = 0;

void announceDataPort(int sockfd){
	int n_bytes = writeBytes(sockfd, 
							(void*)&DATA_PORT_ANNOUNCEMENT, 
							sizeof(DATA_PORT_ANNOUNCEMENT));
	if (n_bytes <= 0){
		print_error("send DATA_PORT_ANNOUNCEMENT to index server");
		exit(1);
	}
	fprintf(stream, "update_file_list.c > dataPort = %u\n", ntohs(dataPort));
	n_bytes = writeBytes(sockfd,
						(void*)&dataPort,
						sizeof(dataPort));
	if (n_bytes <= 0){
		print_error("send data port to index server");
		exit(1);
	}
}

void update_file_list(char *dir_name){
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
}
