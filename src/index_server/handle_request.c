#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "../LinkedList.h"
#include "handle_request.h"
#include "../common.h"
#include "../sockio.h"
#include "LinkedListUtils.h"

struct LinkedList *file_list = NULL;
pthread_mutex_t lock_file_list = PTHREAD_MUTEX_INITIALIZER;

void handleSocketError(struct net_info cli_info, char *mess){
	//print error message
	char err_mess[256];
	sprintf(err_mess, "%s:%u > %s", cli_info.ip_add, cli_info.port, mess);
	print_error(err_mess);

	/* remove the host from the file_list */
	struct DataHost host;
	host.ip_addr = ntohl(inet_addr(cli_info.ip_add));
	host.port = cli_info.data_port;
	removeHost(host);

	fprintf(stderr, "closing connection from %s:%u\n", cli_info.ip_add, cli_info.port);
	close(cli_info.sockfd);
	fprintf(stderr, "connection from %s:%u closed\n", cli_info.ip_add, cli_info.port);

	/* TODO: display file list */

	int ret = 100;
	pthread_exit(&ret);
}

void removeHost(struct DataHost host){
	fprintf(stream, "function: removeHost\n");
	pthread_mutex_lock(&lock_file_list);
	struct Node *it = file_list->head;
	int need_to_remove_the_head = 0;
	for (; it != NULL; it = it->next){
		struct FileOwner *tmp = (struct FileOwner*)it->data;
		struct Node *host_node = getNodeByHost(tmp->host_list, host);
		printf("host_node: %p\n", host_node);
		if (host_node){
			fprintf(stream, "function: removeNode/host\n");
			removeNode(tmp->host_list, host_node);
			//fprintf(stream, "removeNode/host done\n");
			/* if the host_list is empty, also remove the file from file_list */
			if (tmp->host_list->n_nodes <= 0){
				if (it == file_list->head){
					need_to_remove_the_head = 1;
					continue;
				}
				it = it->prev;		//if (it == head) => it->prev == NULL
				fprintf(stream, "function: removeNode/file\n");
				removeNode(file_list, it->next);
				//fprintf(stream, "removeNode/file done\n");
			}
		}
	}
	if (need_to_remove_the_head){
		pop(file_list);
	}
	pthread_mutex_unlock(&lock_file_list);
}
