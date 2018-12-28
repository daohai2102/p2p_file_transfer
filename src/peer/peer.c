#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>

#include "../common.h"
#include "connect_index_server.h"
#include "update_file_list.h"
#include "list_files_request.h"


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
	
	connect_to_index_server();

	pthread_t tid;
	int thr = pthread_create(&tid, NULL, &update_file_list, "./");
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
		} else if (strcmp(command, "get") == 0){
			/* TODO: send list_hosts_request */
		}

		printf("command> ");
	}

	return 0;
}
