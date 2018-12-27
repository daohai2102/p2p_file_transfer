#ifndef _HANDLE_REQUEST_H_
#define _HANDLE_REQUEST_H_

#include <stdint.h>
#include <pthread.h>

#include "../LinkedList.h"

extern struct LinkedList *file_list;
extern pthread_mutex_t lock_file_list;

/* info for each client connected to the index server */
struct net_info{
	int sockfd;
	char ip_add[16];
	uint16_t port;			//client connect to index server
	uint16_t data_port;		//client listen for download_file_request
};

void handleSocketError(struct net_info cli_info, char *mess);

#endif