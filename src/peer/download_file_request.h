#ifndef _DOWNLOAD_FILE_REQUEST_H_
#define _DOWNLOAD_FILE_REQUEST_H_

#include <pthread.h>
#include "../LinkedList.h"

#define MINIMUM_SEGMENT_SIZE	81920	//80KB

const char tmp_dir[] = "./temp/";

extern struct LinkedList *segment_list;
extern pthread_mutex_t lock_segment_list;
extern pthread_cond_t cond_segment_list;

void* download_file(void *arg);

int download_done();


#endif
