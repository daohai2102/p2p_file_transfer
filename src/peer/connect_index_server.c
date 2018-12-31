#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include "../common.h"
#include "../sockio.h"
#include "connect_index_server.h"
#include "list_files_request.h"
#include "list_hosts_request.h"

pthread_mutex_t lock_servsock = PTHREAD_MUTEX_INITIALIZER;

int servsock = 0;

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

	int servsocket = socket(AF_INET, SOCK_STREAM, 0);
	if (servsocket < 0){
		print_error("socket");
		exit(2);
	}

	if (connect(servsocket, (struct sockaddr*) &servsin, sizeof(servsin)) < 0){
		print_error("connect");
		exit(3);
	}

	servsock = servsocket;
	return servsocket;
}

void* process_response(void *arg){
	pthread_detach(pthread_self());
	int servsock = *(int*)arg;

	/* process responses from the index server */
	uint8_t packet_type;
	while (readBytes(servsock, &packet_type, sizeof(packet_type)) > 0){
		if (packet_type == LIST_FILES_RESPONSE){
			process_list_files_response(servsock);
			printf("command> ");	//make the console prettier
			fflush(stdout);
		} else if (packet_type == LIST_HOSTS_RESPONSE){
			fprintf(stream, "list_hosts_response\n");
			process_list_hosts_response();
		}
	}
	return NULL;
}
