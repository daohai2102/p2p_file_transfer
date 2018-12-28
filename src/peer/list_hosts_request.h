#ifndef _LIST_HOSTS_REQUEST_H_
#define _LIST_HOSTS_REQUEST_H_

#include <pthread.h>
#include "../LinkedList.h"


extern struct FileOwner *the_file;
extern pthread_mutex_t lock_the_file;
extern pthread_cond_t cond_the_file;

void send_list_hosts_request(char *filename);

void process_list_hosts_response();
#endif
