#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "download_file_request.h"
#include "list_hosts_request.h"
#include "../common.h"
#include "../sockio.h"


struct LinkedList *segment_list = NULL;
pthread_mutex_t lock_segment_list = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_segment_list = PTHREAD_COND_INITIALIZER;
const char tmp_dir[] = "./.temp/";

static struct Segment* create_segment(uint8_t sequence){
	pthread_mutex_lock(&lock_the_file);
	if (sequence != seq_no){
		pthread_mutex_unlock(&lock_the_file);
		return NULL;
	}
	uint32_t filesize = the_file->filesize;
	pthread_mutex_unlock(&lock_the_file);
	struct Segment *segment = NULL;
	
	pthread_mutex_lock(&lock_segment_list);
	fprintf(stream, "[create_segment] wait to access segment_list\n");
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
				pthread_mutex_unlock(&seg1->lock_seg);
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
		return sockfd;
	}

	int conn = connect(sockfd, (struct sockaddr*)&sin, sizeof(sin));
	if (conn < 0)
		return conn;

	fprintf(stdout, "connected to %s\n", addr_str);
	return sockfd;
}

void* download_file(void *arg){
	fprintf(stream, "function download_file\n");
	pthread_detach(pthread_self());
	struct DownloadInfo dinfo = *(struct DownloadInfo*)arg;
	free(arg);

	char addr_str[22];

	char buff[MAX_BUFF_SIZE];
	uint32_t n_write;
	
	while(1){
		struct Segment *segment = create_segment(dinfo.seq_no);
		if (segment == NULL){
			fprintf(stream, "no more segment need to be downloaded\n");
			pthread_mutex_lock(&lock_the_file);
			uint8_t current_seq = seq_no;
			pthread_mutex_unlock(&lock_the_file);

			/* Emitting this signal means that no more segments need to be downloaded.
			 * if dinfo.seq_no != current_seq:
			 *		Some threads terminated and emitted this signal, 
			 *		the system detected that the file has been downloaded successfully, 
			 *		so no need to emit signal any more */
			if (dinfo.seq_no == current_seq){
				fprintf(stream, "[download_file] emit signal\n");
				pthread_mutex_lock(&lock_segment_list);
				pthread_cond_signal(&cond_segment_list);
				pthread_mutex_unlock(&lock_segment_list);
			}
			fprintf(stream, "[download_file]terminate thread\n");
			terminate_thread(segment);
		}
		int sockfd = connect_peer(dinfo.dthost, addr_str);
		if (sockfd < 0){
			handle_error(segment, addr_str, "connect to download\n");
		}
		fprintf(stream, "%s > segment offset: %u\n", addr_str, segment->offset);

		/* send download file request */
		uint32_t n_bytes = 0;
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

			int filefd = open(full_name, O_WRONLY | O_CREAT, 0664);
			if (filefd < 0){
				print_error("[download_file] open file to save data");
				terminate_thread(segment);
			}


			fprintf(stream, "[download_file] lseek to %u\n", segment->offset);
			uint32_t pos = lseek(filefd, segment->offset, SEEK_SET);
			fprintf(stream, "[download_file] pos after lseeking: %u\n", pos);
			//FILE *file = fopen(full_name, "wb");
			//if (file == NULL){
			//	fprintf(stderr, "%s > [download_file] cannot open file", the_file->filename);
			//	terminate_thread(segment);
			//}
			//fprintf(stream, "[download_file] fseeko to %u\n", segment->offset);
			//fseeko(file, segment->offset, SEEK_SET);
			//fprintf(stream, "[download_file] position after fseeking: %u\n", (uint32_t)ftello(file));

			//char bak_name[256];
			//sprintf(bak_name, "%s%u", tmp_dir, segment->offset);
			//FILE *bak = fopen(bak_name, "wb");

			while (1){
				n_bytes = read(sockfd, buff, sizeof(buff));
				if (n_bytes <= 0){
					close(filefd);
					//fclose(file);
					//fclose(bak);
					handle_error(segment, addr_str, "read data");
				}
				pthread_mutex_lock(&segment->lock_seg);
				uint32_t remain = segment->end - (segment->offset + segment->n_bytes);
				n_bytes = (remain < n_bytes) ? remain : n_bytes;
				n_write = write(filefd, buff, n_bytes);
				//fwrite(buff, 1, n_bytes, bak);
				if (n_write != n_bytes){
					fprintf(stream, "%s > n_write=%u, n_bytes=%u\n", addr_str,n_write, n_bytes);
				}
				segment->n_bytes += n_write;
				if (n_write < 0){
					/* error */
					//fflush(file);
					//fclose(file);
					//fclose(bak);
					close(filefd);
					close(sockfd);
					pthread_mutex_unlock(&segment->lock_seg);
					handle_error(segment, addr_str, "write to file");
				}
				if (segment->offset + segment->n_bytes == segment->end){
					/* got enough data */
					//fclose(file);
					//fclose(bak);
					close(filefd);
					segment->downloading = 0;
					fprintf(stream, "%s > segment offset %u done, n_bytes=%u\n", 
							addr_str, segment->offset, segment->n_bytes);
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
	int file_not_found = 0;
	while(1){
		pthread_cond_wait(&cond_segment_list, &lock_segment_list);
		struct Node *it = segment_list->head;
		if (!it){
			file_not_found = 1;
			pthread_mutex_unlock(&lock_segment_list);
			break;
		}
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

	pthread_mutex_lock(&lock_the_file);
	if (file_not_found){
		fprintf(stdout, "index server > \'%s\' not found\n", the_file->filename);
	} else {
		fprintf(stdout, "received \'%s\' successfully\n", the_file->filename);
	}
	pthread_mutex_unlock(&lock_the_file);

	/* send "done" message to index server */
	send_list_hosts_request("");

	pthread_mutex_lock(&lock_the_file);
	/* increase sequence number */
	seq_no ++;

	/* move the file to the current directory */
	char full_name[400];
	strcpy(full_name, tmp_dir);
	strcat(full_name, the_file->filename);
	rename(full_name, the_file->filename);

	/* destruct the_file and segment_list */
	destructLinkedList(the_file);
	the_file = NULL;
	pthread_mutex_unlock(&lock_the_file);

	pthread_mutex_lock(&lock_segment_list);
	destructLinkedList(segment_list);
	segment_list = NULL;
	pthread_mutex_unlock(&lock_segment_list);
	
	return 1;
}
