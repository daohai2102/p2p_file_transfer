#ifndef _DOWNLOAD_FILE_REQUEST_H_
#define _DOWNLOAD_FILE_REQUEST_H_

#include <pthread.h>
#include "../LinkedList.h"

#define MINIMUM_SEGMENT_SIZE	81920	//80KB

extern const char tmp_dir[];
extern struct LinkedList *segment_list;
extern int download_file_done;
extern pthread_mutex_t lock_segment_list;
extern pthread_cond_t cond_segment_list;

struct DownloadInfo{
	struct DataHost dthost;
	uint8_t seq_no;
};

void* download_file(void *arg);

int download_done();


#endif
