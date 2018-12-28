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

static void displayFileListFromHost(uint32_t ip_addr, uint16_t port){
	printf("%-4s | %-25s | %-50s | %-15s\n", "No", "Host", "Filename", "Filesize (byte)");
	pthread_mutex_lock(&lock_file_list);
	struct Node *it = file_list->head;
	int k = 0;
	for (; it != NULL; it = it->next){
		struct FileOwner *fo = (struct FileOwner*)it->data;
		//printf("file \'%s\'\n", fo->filename);
		//if (fo->host_list == NULL){
		//	printf("fo->host_list == NULL\n");
		//}
		struct Node *it2 = fo->host_list->head;
		for (; it2 != NULL; it2 = it2->next){
			struct DataHost *dh = (struct DataHost*)it2->data;
			//printf("(%u:%u) vs (%u:%u)\n", host.ip_addr, host.port, dh->ip_addr, dh->port);
			if (ip_addr == dh->ip_addr && port == dh->port){
				char delim[104];
				memset(delim, '.', 103);
				delim[103] = 0;
				printf("%s\n", delim);
				char host[22];
				struct in_addr addr;
				addr.s_addr = htonl(dh->ip_addr);
				sprintf(host, "%s:%u", inet_ntoa(addr), dh->port);
				k++;
				printf("%-4d | %-25s | %-50s | %-15u\n", k, host, fo->filename, fo->filesize);
				break;
			}
		}
	}
	pthread_mutex_unlock(&lock_file_list);
	printf("\n");
}

static void displayFileList(){
	printf("%-4s | %-25s | %-50s | %-15s\n", "No", "Host", "Filename", "Filesize (byte)");
	pthread_mutex_lock(&lock_file_list);
	if (file_list == NULL){
		fprintf(stderr, "file_list == NULL\n");
		return;
	}
	struct Node *it = file_list->head;
	int k = 0;
	for (; it != NULL; it = it->next){
		struct FileOwner *fo = (struct FileOwner*)it->data;
		if (fo->host_list == NULL){
			fprintf(stderr, "\'%s\' has fo->host_list == NULL\n", fo->filename);
			continue;
		}
		struct Node *it2 = fo->host_list->head;
		for (; it2 != NULL; it2 = it2->next){
			k++;
			char delim[104];
			memset(delim, '.', 103);
			delim[103] = 0;
			printf("%s\n", delim);
			struct DataHost *dh = (struct DataHost*)it2->data;
			char host[22];
			struct in_addr addr;
			addr.s_addr = htonl(dh->ip_addr);
			sprintf(host, "%s:%u", inet_ntoa(addr), dh->port);
			printf("%-4d | %-25s | %-50s | %-15u\n", k, host, fo->filename, fo->filesize);
		}
	}
	pthread_mutex_unlock(&lock_file_list);
	printf("\n");
}

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

	/* display file list */
	fprintf(stdout, "File list after removing the host: \n");
	displayFileList();

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
	int changed = 0;
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
			changed = 1;
			/* add the host to the list */
			struct Node *host_node = newNode(&host, DATA_HOST_TYPE);

			//check if the file has already been in the list
			if (llContainFile(file_list, filename)){
				fprintf(stream, "\'%s\' existed, add host to the list\n", filename);
				/* insert the host into the host_list of the file
				 */

				//get the node which contains the file in the file_list
				struct Node *node = getNodeByFilename(file_list, filename);
				struct FileOwner *file = (struct FileOwner*)node->data;

				/* TODO: need to check if the host already existed */
				push(file->host_list, host_node);
			} else {
				fprintf(stream, "add \'%s\' to file_list, add host to host_list\n", filename);
				//create a new node to store file's info
				struct FileOwner new_file;
				strcpy(new_file.filename, filename);
				fprintf(stream, "%s > new_file->filename: %s\n", cli_addr, new_file.filename);
				new_file.filesize = filesize;
				fprintf(stream, "%s > new_file->filesize: %u\n", cli_addr, new_file.filesize);
				new_file.host_list = newLinkedList();
				fprintf(stream, "%s > new_file->host_list: %p\n", cli_addr, new_file.host_list);
				push(new_file.host_list, host_node);
				fprintf(stream, "%s > new_file->host_list->head: %p\n", 
						cli_addr, new_file.host_list->head);
				struct Node *file_node = newNode(&new_file, FILE_OWNER_TYPE);
				push(file_list, file_node);
			}

			fprintf(stdout, "%s > added a new file: %s\n", 
					cli_addr, filename);
		} else if (status == FILE_DELETED){
			changed = 1;
			/* remove the host from the list */
			//remove the host from the host_list of the file
			/* TODO: need to check if the file really exist in the file_list
			 * TODO: need to check if the host really exist in the host_list of the file */
			struct Node *file_node = getNodeByFilename(file_list, filename);
			struct FileOwner *file = (struct FileOwner*)(file_node->data);
			struct Node *host_node = getNodeByHost(file->host_list, host);
			removeNode(file->host_list, host_node);
			/* if the host_list is empty, also remove the file from file_list */
			if (file->host_list->n_nodes <= 0){
				removeNode(file_list, file_node);
			}
			fprintf(stdout, "%s > deleted a file: %s\n", cli_addr, filename);
		}
	}
	if (changed){
		fprintf(stdout, "%s > file_list after updating:\n", cli_addr);
		displayFileListFromHost(ntohl(inet_addr(cli_info.ip_add)), cli_info.data_port);
	}
}

void process_list_files_request(struct net_info cli_info){
	char cli_addr[22];
	sprintf(cli_addr, "%s:%u", cli_info.ip_add, cli_info.port);
	fprintf(stream, "%s > list_files_request\n", cli_addr);

	/* TODO: send response header */
	long n_bytes;
	n_bytes = writeBytes(cli_info.sockfd, 
						(void*)&LIST_FILES_RESPONSE, 
						sizeof(LIST_FILES_RESPONSE));
	if (n_bytes <= 0){
		handleSocketError(cli_info, "send LIST_FILES_RESPONSE message");
	}
	
	uint8_t n_files = 0;

	if (file_list == NULL)
		n_files = 0;
	else 
		n_files = file_list->n_nodes;

	n_bytes = writeBytes(cli_info.sockfd, 
						&n_files, 
						sizeof(n_files));
	if (n_bytes <= 0){
		handleSocketError(cli_info, "send n_files");
	}

	if (n_files == 0)
		return;

	struct Node *it = file_list->head;
	for (; it != NULL; it = it->next){
		struct FileOwner *file = (struct FileOwner*)it->data;
		uint16_t filename_length = strlen(file->filename) + 1;
		filename_length = htons(filename_length);
		n_bytes = writeBytes(cli_info.sockfd, &filename_length, sizeof(filename_length));
		if (n_bytes <= 0){
			handleSocketError(cli_info, "send filename_length");
		}
		
		n_bytes = writeBytes(cli_info.sockfd, file->filename, ntohs(filename_length));
		if (n_bytes <= 0){
			handleSocketError(cli_info, "send filename");
		}
	}
}
