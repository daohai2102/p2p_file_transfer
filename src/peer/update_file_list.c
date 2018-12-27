#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include "update_file_list.h"
#include "../common.h"
#include "../sockio.h"


uint16_t dataPort = 0;

void announceDataPort(int sockfd){
	int n_bytes = writeBytes(sockfd, 
							(void*)&DATA_PORT_ANNOUNCEMENT, 
							sizeof(DATA_PORT_ANNOUNCEMENT));
	if (n_bytes <= 0){
		print_error("send DATA_PORT_ANNOUNCEMENT to index server");
		exit(1);
	}
	fprintf(stream, "update_file_list.c > dataPort = %u\n", ntohs(dataPort));
	n_bytes = writeBytes(sockfd,
						(void*)&dataPort,
						sizeof(dataPort));
	if (n_bytes <= 0){
		print_error("send data port to index server");
		exit(1);
	}
}

static void send_file_list(int sockfd, struct FileStatus *fs, uint8_t n_fs){
	return;
}

void update_file_list(char *dir_name){
	/* connect to index server */
	struct sockaddr_in servsin;
	bzero(&servsin, sizeof(servsin));

	servsin.sin_family = AF_INET;

	char *servip;
	char buf[22];
	printf("Enter server's IP addr and port in the form of <IP>:<port>\n");
	scanf("%s", buf);
	getchar();	/* remove '\n' character from the stdin */

	/* parse the buf to get ip address and port */
	servip = strtok(buf, ":");
	servsin.sin_addr.s_addr = inet_addr(servip);
	servsin.sin_port = htons(atoi(strtok(NULL, ":")));

	int servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock < 0){
		print_error("socket");
		exit(2);
	}

	if (connect(servsock, (struct sockaddr*) &servsin, sizeof(servsin)) < 0){
		print_error("connect");
		exit(3);
	}

	//data_port_announcement must be sent first
	announceDataPort(servsock);

	/* list files in a directory */
	struct FileStatus fs[20];
	uint8_t n_fs = 0;
	DIR *dir;
	struct dirent *ent;
	dir = opendir(dir_name);
	if (dir){
		while ((ent = readdir(dir)) != NULL){
			if (ent->d_name[0] == '.')
				continue;
			fprintf(stdout, "new file: %s\n", ent->d_name);
			strcpy(fs[n_fs].filename, ent->d_name);
			fprintf(stream, "fs[n_fs].filename: %s\n", fs[n_fs].filename);
			long sz = getFileSize(ent->d_name);
			fprintf(stream, "filesize: %ld\n", sz);
			if (sz < 0)
				continue;
			fs[n_fs].filesize = sz;
			fs[n_fs ++].status = FILE_NEW;
		}
		closedir(dir);

		if (n_fs > 0)
			send_file_list(servsock, fs, n_fs);
		else
			fprintf(stdout, "Directory %s is empty\n", dir_name);
		
	} else {
		print_error("opendir");
		exit(1);
	}
}