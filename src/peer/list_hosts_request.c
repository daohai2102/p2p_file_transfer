#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include "list_hosts_request.h"
#include "../common.h"
#include "../sockio.h"
#include "../LinkedListUtils.h"
#include "connect_index_server.h"
#include "download_file_request.h"

struct FileOwner *the_file = NULL;
pthread_mutex_t lock_the_file = PTHREAD_MUTEX_INITIALIZER;
//pthread_cond_t cond_the_file = PTHREAD_COND_INITIALIZER;

void send_list_hosts_request(char *filename){
	pthread_mutex_lock(&lock_servsock);
	fprintf(stream, "[send_list_hosts_request] filename: \'%s\'\n", filename);

	int n_bytes = writeBytes(servsock, 
							(void*)&LIST_HOSTS_REQUEST, 
							sizeof(LIST_HOSTS_REQUEST));
	if (n_bytes <= 0){
		print_error("send LIST_HOSTS_REQUEST to index server");
		exit(1);
	}
	uint16_t filename_length = strlen(filename);
	if (filename_length != 0){
		filename_length += 1;
	}
	filename_length = htons(filename_length);

	n_bytes = writeBytes(servsock, &filename_length, sizeof(filename_length));
	if (n_bytes <= 0){
		print_error("list_hosts_request > send filename_length");
		exit(1);
	}

	if (filename_length > 0){
		n_bytes = writeBytes(servsock, filename, ntohs(filename_length));
		if (n_bytes <= 0){
			print_error("list_hosts_request > send filename");
			exit(1);
		}
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
	fprintf(stream, "exec process_list_hosts_response\n");

	long n_bytes;

	uint16_t filename_length;
	n_bytes = readBytes(servsock, &filename_length, sizeof(filename_length));
	if (n_bytes <= 0){
		print_error("[process_list_hosts_response]read filename_length");
		exit(1);
	}
	filename_length = ntohs(filename_length);
	fprintf(stream, "[process_list_hosts_response]filename_length: %u\n", filename_length);

	char filename[256];
	n_bytes = readBytes(servsock, filename, filename_length);
	if (n_bytes <= 0){
		print_error("[process_list_hosts_response]read filename");
		exit(1);
	}
	fprintf(stream, "[process_list_hosts_response]filename: %s\n", filename);
	
	uint32_t filesize;
	n_bytes = readBytes(servsock, &filesize, sizeof(filesize));
	if (n_bytes <= 0){
		print_error("process_list_hosts_response > read filesize");
		exit(1);
	}
	filesize = ntohl(filesize);
	fprintf(stream, "[process_list_hosts_response]filesize: %u\n", filesize);

	int download_done = 1;
	if (the_file && strcmp(filename, the_file->filename) == 0){
		download_done = 0;
	}

	if (!download_done)
		the_file->filesize = filesize;

	pthread_mutex_lock(&lock_the_file);
	
	uint8_t n_hosts;
	n_bytes = readBytes(servsock, &n_hosts, sizeof(n_hosts));
	if (n_bytes <= 0){
		print_error("[process_list_hosts_response]read n_hosts");
		exit(1);
	}
	fprintf(stream, "[process_list_hosts_response]n_hosts: %u\n", n_hosts);

	uint8_t i = 0;
	for (; i < n_hosts; i++){
		uint8_t status;
		n_bytes = readBytes(servsock, &status, sizeof(status));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read status");
			exit(1);
		}
		fprintf(stream, "[process_list_hosts_response]status: %u\n", status);
		
		uint32_t ip_addr;
		n_bytes = readBytes(servsock, &ip_addr, sizeof(ip_addr));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read ip address");
			exit(1);
		}
		ip_addr = ntohl(ip_addr);
		fprintf(stream, "[process_list_hosts_response]ip_addr: %u\n", ip_addr);
		
		uint16_t data_port;
		n_bytes = readBytes(servsock, &data_port, sizeof(data_port));
		if (n_bytes <= 0){
			print_error("process_list_hosts_response > read data port");
			exit(1);
		}
		data_port = ntohs(data_port);
		fprintf(stream, "[process_list_hosts_response]data_port: %u\n", data_port);
		
		if (!download_done){
			struct DataHost *dthost = malloc(sizeof(struct DataHost));
			dthost->ip_addr = ip_addr;
			dthost->port = data_port;
			if (status == FILE_NEW){
				struct Node *host_node = getNodeByHost(the_file->host_list, *dthost);
				if (!host_node){
					fprintf(stream, "[process_list_hosts_response]new host, add to the list\n");
					host_node = newNode(dthost, DATA_HOST_TYPE);
					push(the_file->host_list, host_node);
					/* create new thread to download file */
					pthread_t tid;
					int thr = pthread_create(&tid, NULL, &download_file, dthost);
					if (thr != 0){
						fprintf(stream, "cannot create new thread to download file\n");
						free(dthost);
						continue;
					}
					fprintf(stream, "[process_list_hosts_response]created new thread to download file\n");
				}
			} else if (status == FILE_DELETED) {
				struct Node *host_node = getNodeByHost(the_file->host_list, *dthost);
				free(dthost);
				if (host_node){
					removeNode(the_file->host_list, host_node);
					fprintf(stream, "[process_list_hosts_response]host removed\n");
				} else {
					fprintf(stream, "[process_list_hosts_response]host is not existed\n");
				}
			}
		}
	}

	pthread_mutex_unlock(&lock_the_file);

	if (!download_done)
		display_host_list();
}
