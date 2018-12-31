#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "download_file_request.h"
#include "list_hosts_request.h"
#include "../common.h"
#include "../sockio.h"


struct LinkedList *segment_list = NULL;
int download_file_done = 1;
pthread_mutex_t lock_segment_list = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_segment_list = PTHREAD_COND_INITIALIZER;
const char tmp_dir[] = "./.temp/";

static struct Segment* create_segment(){
	pthread_mutex_lock(&lock_the_file);
	uint32_t filesize = the_file->filesize;
	pthread_mutex_unlock(&lock_the_file);
	struct Segment *segment = NULL;
	
	pthread_mutex_lock(&lock_segment_list);
	pthread_cleanup_push(mutex_unlock, &lock_segment_list);
	if (segment_list->n_nodes == 0){
		struct Segment *seg = malloc(sizeof(struct Segment));
		struct Node *seg_node = newNode(seg, SEGMENT_TYPE);
		free(seg);
		seg = (struct Segment*)(seg_node->data);
		segment = seg;
		seg->downloading = 1;
		pthread_mutex_init(&seg->lock_seg, NULL);
		seg->n_bytes = 0;
		seg->offset = 0;
		seg->end = filesize;
		push(segment_list, seg_node);

		seg = malloc(sizeof(struct Segment));
		seg_node = newNode(seg, SEGMENT_TYPE);
		free(seg);
		seg = (struct Segment*)(seg_node->data);
		seg->downloading = 0;
		pthread_mutex_init(&seg->lock_seg, NULL);
		seg->n_bytes = 0;
		seg->offset = filesize;
		seg->end = filesize;
		push(segment_list, seg_node);
	} else {
		struct Node *it = segment_list->head;
		for (; it != segment_list->tail; it = it->next){
			struct Segment *seg1 = (struct Segment*)(it->data);

			pthread_mutex_lock(&seg1->lock_seg);
			if (seg1->downloading == 0 && (seg1->offset + seg1->n_bytes) < seg1->end){
				segment = seg1;
				break;
			}
			pthread_mutex_unlock(&seg1->lock_seg);

			struct Segment *seg2 = (struct Segment*)(it->next->data);
			pthread_mutex_lock(&seg1->lock_seg);
			pthread_mutex_lock(&seg2->lock_seg);
			uint32_t interval = seg2->offset - (seg1->offset + seg1->n_bytes);
			if (interval >= 2*MINIMUM_SEGMENT_SIZE){
				struct Node *seg_node = newNode(seg1, SEGMENT_TYPE);
				segment = (struct Segment*)(seg_node->data);
				segment->downloading = 1;
				segment->n_bytes = 0;
				segment->offset = (seg2->offset + seg1->offset + seg1->n_bytes)/2;
				pthread_mutex_init(&segment->lock_seg, NULL);
				segment->end = seg2->offset;
				insertNode(segment_list, seg_node, it);
				seg1->end = segment->offset;
			}
			pthread_mutex_unlock(&seg2->lock_seg);
			pthread_mutex_unlock(&seg1->lock_seg);
			if (interval >= 2*MINIMUM_SEGMENT_SIZE){
				break;
			}
		}
	}
	struct Node *it = segment_list->head;
	for (; it != NULL; it = it->next){
		struct Segment *seg = (struct Segment*)(it->data);
		fprintf(stream, "seg: offset=%u, n_bytes=%u, end=%u, downloading=%d\n",
				seg->offset, seg->n_bytes, seg->end, seg->downloading);
	}
	
	pthread_cleanup_pop(0);
	pthread_mutex_unlock(&lock_segment_list);
	return segment;
}

static void terminate_thread(struct Segment *seg){
	if (seg){
		pthread_mutex_lock(&seg->lock_seg);
		seg->downloading = 0;
		pthread_mutex_unlock(&seg->lock_seg);
	}
	int ret = 100;
	pthread_exit(&ret);
}

static void handle_error(struct Segment *seg, char *addr_str, char *mess){
	char err_mess[256];
	sprintf(err_mess, "%s > %s", addr_str, mess);
	print_error(err_mess);
	terminate_thread(seg);
}

static int connect_peer(struct DataHost dthost, char *addr_str){
	struct sockaddr_in sin;
	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(dthost.ip_addr);
	sin.sin_port = htons(dthost.port);

	sprintf(addr_str, "%s:%u", inet_ntoa(sin.sin_addr), dthost.port);

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		handle_error(NULL, addr_str, "create socket");
	}

	if (connect(sockfd, (struct sockaddr*)&sin, sizeof(sin)) < 0){
		handle_error(NULL, addr_str, "connect");
	}

	fprintf(stdout, "connected to %s\n", addr_str);
	return sockfd;
}

void* download_file(void *arg){
	pthread_detach(pthread_self());
	struct DataHost dthost = *(struct DataHost*)arg;
	free(arg);

	char addr_str[22];

	char buff[MAX_BUFF_SIZE];
	uint32_t n_write;
	
	while(1){
		int sockfd = connect_peer(dthost, addr_str);
		struct Segment *segment = create_segment();
		if (segment == NULL){
			pthread_mutex_lock(&lock_segment_list);
			pthread_cond_signal(&cond_segment_list);
			pthread_mutex_unlock(&lock_segment_list);
			close(sockfd);
			terminate_thread(segment);
		}

		/* send download file request */
		long n_bytes = 0;
		uint16_t filename_length = strlen(the_file->filename) + 1;
		filename_length = htons(filename_length);
		n_bytes = writeBytes(sockfd, &filename_length, sizeof(filename_length));
		if (n_bytes <= 0){
			handle_error(segment, addr_str, "send filename_length");
		}

		n_bytes = writeBytes(sockfd, the_file->filename, ntohs(filename_length));
		if (n_bytes <= 0){
			handle_error(segment, addr_str, "send filename");
		}

		uint32_t offset = htonl(segment->offset);
		n_bytes = writeBytes(sockfd, &offset, sizeof(offset));
		if (n_bytes <= 0){
			handle_error(segment, addr_str, "send offset");
		}

		/* receive file status message */
		uint8_t file_status;
		n_bytes = readBytes(sockfd, &file_status, sizeof(file_status));
		if (n_bytes <= 0){
			handle_error(segment, addr_str, "read file status");
		}
		
		if (file_status == FILE_NOT_FOUND){
			fprintf(stdout, "%s > file not found\n", addr_str);
			close(sockfd);
			terminate_thread(segment);
		} else if (file_status == OPENING_FILE_ERROR){
			fprintf(stdout, "%s > opening file error\n", addr_str);
			close(sockfd);
			terminate_thread(segment);
		} else if (file_status == READY_TO_SEND_DATA){
			/* open file to save data */
			char full_name[400];
			strcpy(full_name, tmp_dir);
			strcat(full_name, the_file->filename);
			FILE *file = fopen(full_name, "wb");
			if (file == NULL){
				fprintf(stderr, "%s > [download_file] cannot open file", the_file->filename);
				terminate_thread(segment);
			}
			fseeko(file, segment->offset, 0);

			while (1){
				n_bytes = read(sockfd, buff, sizeof(buff));
				if (n_bytes <= 0){
					fclose(file);
					handle_error(segment, addr_str, "read data");
				}
				pthread_mutex_lock(&segment->lock_seg);
				uint32_t remain = segment->end - (segment->offset + segment->n_bytes);
				n_bytes = (remain < n_bytes) ? remain : n_bytes;
				n_write = fwrite(buff, 1, n_bytes, file);
				segment->n_bytes += n_write;
				if (n_write < n_bytes){
					/* error */
					fflush(file);
					fclose(file);
					close(sockfd);
					fprintf(stderr, "%s > error when writing to file\n", addr_str);
					pthread_mutex_unlock(&segment->lock_seg);
					terminate_thread(segment);
				}
				if (n_write == remain){
					/* got enough data */
					fclose(file);
					segment->downloading = 0;
					fprintf(stream, "%s > segment offset %u done\n", addr_str, segment->offset);
					pthread_mutex_unlock(&segment->lock_seg);
					break;
				}
				pthread_mutex_unlock(&segment->lock_seg);
			}
			close(sockfd);
		}
	}
	return NULL;
}

int download_done(){
	/* check if the file has been downloaded successfully */
	while(1){
		pthread_cond_wait(&cond_segment_list, &lock_segment_list);
		struct Node *it = segment_list->head;
		int done = 1;
		for (; it != segment_list->tail; it = it->next){
			struct Segment *seg = (struct Segment*)(it->data);
			if (seg->downloading || seg->end != (seg->offset + seg->n_bytes)){
				done = 0;
				break;
			}
		}
		pthread_mutex_unlock(&lock_segment_list);
		if (done)
			break;
	}
	/* send "done" message to index server */
	send_list_hosts_request("");

	/* destruct the_file and segment_list */
	pthread_mutex_lock(&lock_the_file);
	char full_name[400];
	strcpy(full_name, tmp_dir);
	strcat(full_name, the_file->filename);
	int ren = rename(full_name, the_file->filename);
	if (ren < 0){
		print_error("move file");
		exit(1);
	}
	destructLinkedList(the_file);
	the_file = NULL;
	pthread_mutex_unlock(&lock_the_file);
	
	return 1;
}
