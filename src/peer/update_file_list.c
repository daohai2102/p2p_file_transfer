#include <stdlib.h>
#include <arpa/inet.h>

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
