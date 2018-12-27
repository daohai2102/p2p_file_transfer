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

void update_file_list(struct net_info cli_info){
	if (! file_list){
		file_list = newLinkedList();
	}

	char cli_addr[22];

	sprintf(cli_addr, "%s:%u", cli_info.ip_add, cli_info.port);

	fprintf(stderr, "%s > file_list_update\n", cli_addr);

	long n_bytes;
	uint8_t n_files;
	n_bytes = readBytes(cli_info.sockfd, &n_files, sizeof(n_files));
	if (n_bytes <= 0){
		handleSocketError(cli_info, "read n_files from socket");
		exit(1);
	}
	fprintf(stream, "%s > n_files: %u\n", cli_addr, n_files);
	uint8_t i = 0;
	for (; i < n_files; i++){
		//file status
		uint8_t status;
		n_bytes = readBytes(cli_info.sockfd, &status, sizeof(status));
		if (n_bytes <= 0){
			handleSocketError(cli_info, "read status from socket");
		}
		fprintf(stream, "%s > status: %u\n", cli_addr, status);

		//filename length
		uint16_t filename_length;
		n_bytes = readBytes(cli_info.sockfd, &filename_length, sizeof(filename_length));
		if (n_bytes <= 0){
			handleSocketError(cli_info, "read filename_length from socket");
		}
		filename_length = ntohs(filename_length);
		fprintf(stream, "%s > filename_length: %u\n", cli_addr, filename_length);

		//filename
		char *filename = malloc(filename_length);
		n_bytes = readBytes(cli_info.sockfd, filename, filename_length);
		if (n_bytes <= 0){
			handleSocketError(cli_info, "read filename from socket"); 
		} 
		fprintf(stream, "%s > filename: %s\n", cli_addr, filename);

		//filesize
		uint32_t filesize;
		n_bytes = readBytes(cli_info.sockfd, &filesize, sizeof(filesize));
		if (n_bytes <= 0){
			handleSocketError(cli_info, "read filesize from socket");
		}
		filesize = ntohl(filesize);
		fprintf(stream, "%s > filesize: %u\n", cli_addr, filesize);
		
		//struct DataHost *host = malloc(sizeof(struct DataHost));
		struct DataHost host;
		host.ip_addr = ntohl(inet_addr(cli_info.ip_add));
		host.port = cli_info.data_port;

		if (status == FILE_NEW){
			/* TODO: add the host to the list */
		} else if (status == FILE_DELETED){
			/* TODO: remove the host from the list */
		}
	}
}
