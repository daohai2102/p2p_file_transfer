#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "list_hosts_request.h"
#include "../common.h"
#include "../sockio.h"
#include "connect_index_server.h"

struct FileOwner *the_file = NULL;
pthread_mutex_t lock_the_file = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond_the_file = PTHREAD_COND_INITIALIZER;

void send_list_hosts_request(char *filename){
	pthread_mutex_lock(&lock_servsock);

	int n_bytes = writeBytes(servsock, 
							(void*)&LIST_HOSTS_REQUEST, 
							sizeof(LIST_HOSTS_REQUEST));
	if (n_bytes <= 0){
		print_error("send LIST_HOSTS_REQUEST to index server");
		exit(1);
	}
	uint16_t filename_length = strlen(filename) + 1;
	filename_length = htons(filename_length);

	n_bytes = writeBytes(servsock, &filename_length, sizeof(filename_length));
	if (n_bytes <= 0){
		print_error("list_hosts_request > send filename_length");
		exit(1);
	}

	n_bytes = writeBytes(servsock, filename, ntohs(filename_length));
	if (n_bytes <= 0){
		print_error("list_hosts_request > send filename");
		exit(1);
	}

	pthread_mutex_unlock(&lock_servsock);
}

static void display_host_list(){
	printf("%-3s | %-22s\n", "No", "Host (ip:data_port)");
	char delim[29];
	memset(delim, '.', 28);
	delim[28] = 0;
	struct Node *it = the_file->host_list->head;
	int i = 0;
	for(; it != NULL; it = it->next){
		i++;
		struct DataHost *dthost = (struct DataHost*)(it->data);
		struct in_addr addr;
		addr.s_addr = htonl(dthost->ip_addr);
		char *ip_addr = inet_ntoa(addr);
		char addr_full[22];
		sprintf(addr_full, "%s:%u", ip_addr, dthost->port);
		printf("%s\n", delim);
		printf("%-3d | %-22s\n", i, addr_full);
	}
}

void process_list_hosts_response(){
	if (!the_file){
		the_file = malloc(sizeof(struct FileOwner));
		the_file->host_list = newLinkedList();
		/* TODO: start downloading file */
	}
	
	uint32_t filesize;
	long n_bytes = readBytes(servsock, &filesize, sizeof(filesize));
	if (n_bytes <= 0){
		print_error("process_list_hosts_response > read filesize");
		exit(1);
	}
	
	filesize = ntohs(filesize);
	the_file->filesize = filesize;
	
	uint8_t n_hosts = 0;
	n_bytes = readBytes(servsock, &n_hosts, sizeof(n_hosts));
	if (n_bytes <= 0){
		print_error("process_list_hosts_response > read n_hosts");
		exit(1);
	}

	uint8_t i = 0;
	for (; i < n_hosts; i++){
		uint8_t status;
		n_bytes = readBytes(servsock, &status, sizeof(status));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read status");
			exit(1);
		}
		
		uint32_t ip_addr;
		n_bytes = readBytes(servsock, &ip_addr, sizeof(ip_addr));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read ip address");
			exit(1);
		}
		ip_addr = ntohl(ip_addr);
		
		uint16_t data_port;
		n_bytes = readBytes(servsock, &data_port, sizeof(data_port));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read data port");
			exit(1);
		}
		data_port = ntohs(data_port);
		
		struct DataHost dthost;
		dthost.ip_addr = ip_addr;
		dthost.port = data_port;
		struct Node *host_node = newNode(&dthost, DATA_HOST_TYPE);

		if (status == FILE_NEW){
			pthread_mutex_lock(&lock_the_file);
			push(the_file->host_list, host_node);
			//pthread_cond_signal(&cond_the_file);
			pthread_mutex_unlock(&lock_the_file);
			/* TODO: create new thread to download file */
		} else if (status == FILE_DELETED) {
			removeNode(the_file->host_list, host_node);
		}
	}

	display_host_list();
}
