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

	/* TODO: remove the host from the file_list */

	fprintf(stderr, "closing connection from %s:%u\n", cli_info.ip_add, cli_info.port);
	close(cli_info.sockfd);
	fprintf(stderr, "connection from %s:%u closed\n", cli_info.ip_add, cli_info.port);

	/* TODO: display file list */

	int ret = 100;
	pthread_exit(&ret);
}

