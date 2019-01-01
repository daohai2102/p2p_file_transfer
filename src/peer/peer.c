#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../common.h"
#include "connect_index_server.h"
#include "update_file_list.h"
#include "list_files_request.h"
#include "list_hosts_request.h"
#include "download_file_request.h"
#include "handle_download_file_request.h"

const char EXIT_CMD[] = "exit";

int main(int argc, char **argv){
	if (argc > 1){
		if (!strcmp(argv[1], "--debug"))
			stream = stderr;
		else{
			printf("unkown option: %s\n", argv[1]);
			exit(1);
		}
	}else
		stream = fopen("/dev/null", "w");

	/* ignore EPIPE to avoid crashing when writing to a closed socket */
	signal (SIGPIPE, SIG_IGN);

	int dataSock = socket(AF_INET, SOCK_STREAM, 0);
	if (dataSock < 0){
		print_error("create dataSock");
		exit(1);
	}
	struct sockaddr_in dataSockIn;
	bzero(&dataSockIn, sizeof(dataSockIn));
	dataSockIn.sin_addr.s_addr = htonl(INADDR_ANY);
	dataSockIn.sin_family = AF_INET;
	dataSockIn.sin_port = 0;	//let the OS chooses the port
	
	if ((bind(dataSock, (struct sockaddr*) &dataSockIn, sizeof(dataSockIn))) < 0){
		print_error("bind dataSock");
		exit(1);
	}

	if ((listen(dataSock, 20)) < 0){
		print_error("listen on dataSock");
		exit(1);
	}
	
	struct sockaddr_in real_sock_in;	//use to store the listening port
	socklen_t socklen = sizeof(real_sock_in);

	/* get the current socket info (include listening port) */
	if ((getsockname(dataSock, (struct sockaddr*)&real_sock_in, &socklen) < 0)){
		print_error("getsockname");
		exit(1);
	}

	/* port used to listen for download_file_request */
	dataPort = real_sock_in.sin_port;	//network byte order
	//fprintf(stream, "main > dataPort: %u\n", ntohs(dataPort));
	
	/* listen for download file request */
	pthread_t download_tid;
	int thr = pthread_create(&download_tid, NULL, waitForDownloadRequest, &dataSock);
	if (thr != 0){
		print_error("new thread to handle download file request");
		exit(1);
	}
	
	connect_to_index_server();

	pthread_t tid;
	thr = pthread_create(&tid, NULL, &update_file_list, "./");
	if (thr != 0){
		print_error("new thread to update file list to the index server");
		exit(1);
	}

	/* listen for and process responses from the index server */
	pthread_t process_response_tid;
	thr = pthread_create(&process_response_tid, NULL, &process_response, (void*)&servsock);
	if (thr != 0){
		print_error("new thread to process response from the index server");
		close(servsock);
		exit(1);
	}

	mkdir(tmp_dir, 0700);

	printf("command> ");

	while(1){
		char input[400];
		fgets(input, sizeof(input), stdin);

		char *command = strtok(input, " \n\t");

		if (command == NULL){
			printf("command> ");
			continue;
		}

		if (strcmp(command, "ls") == 0){
			send_list_files_request();
		} else if (strcmp(command, "rm") == 0) {
			char *filename = strtok(NULL, " \n\t");
			if (filename){
				int ret = remove(filename);
				if (ret != 0){
					print_error("remove file");
				}
			} else {
				printf("help: rm <filename>\n");
			}
		} else if (strcmp(command, "get") == 0){
			char *filename = strtok(NULL, " \n\t");
			if (filename){
				if (access(filename, F_OK) != -1){
					fprintf(stdout, "\'%s\' existed\n", filename);
				} else {
					pthread_mutex_lock(&lock_the_file);
					the_file = malloc(sizeof(struct FileOwner));
					the_file->host_list = newLinkedList();
					strcpy(the_file->filename, filename);
					the_file->filesize = 0;
					pthread_mutex_unlock(&lock_the_file);

					pthread_mutex_lock(&lock_segment_list);
					segment_list = newLinkedList();
					pthread_mutex_unlock(&lock_segment_list);

					pthread_mutex_lock(&lock_n_threads);
					n_threads = 0;
					pthread_mutex_unlock(&lock_n_threads);

					send_list_hosts_request(filename);

					download_done();

				}
			} else {
				printf("help: get <filename>\n");
			}
		} else if (strcmp(command, EXIT_CMD) == 0){
			return 0;
		}

		printf("command> ");
		fflush(stdout);
	}

	return 0;
}
