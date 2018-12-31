#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <stdio.h>

#define FILE_NEW 0
#define FILE_DELETED 1

#define PACKET_TYPE_SIZE 1

/* request types */
extern const uint8_t DATA_PORT_ANNOUNCEMENT;
extern const uint8_t FILE_LIST_UPDATE;
extern const uint8_t LIST_FILES_REQUEST;
extern const uint8_t LIST_FILES_RESPONSE;
extern const uint8_t LIST_HOSTS_REQUEST;
extern const uint8_t LIST_HOSTS_RESPONSE;

/* request status types */
extern const uint8_t READY_TO_SEND_DATA;
extern const uint8_t FILE_NOT_FOUND;
extern const uint8_t OPENING_FILE_ERROR;

extern FILE *stream;

uint32_t getFileSize(char *filename);

int print_error(char *mess);

void free_mem(void *arg);

void mutex_unlock(void *arg);

void cancel_thread(void *arg);

#endif
