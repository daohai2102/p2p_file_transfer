#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "common.h"

const uint8_t DATA_PORT_ANNOUNCEMENT = 0;
const uint8_t FILE_LIST_UPDATE = 1;
const uint8_t LIST_FILES_REQUEST = 2;
const uint8_t LIST_FILES_RESPONSE = 3;
const uint8_t LIST_HOSTS_REQUEST = 4;
const uint8_t LIST_HOSTS_RESPONSE = 5;

const uint8_t READY_TO_SEND_DATA = 0;
const uint8_t FILE_NOT_FOUND = 1;
const uint8_t OPENING_FILE_ERROR = 2;

FILE *stream = NULL;

long getFileSize(char *filename){
	FILE *file = fopen(filename, "rb");
	char err_mess[256];
	if (!file){
		strerror_r(errno, err_mess, sizeof(err_mess));
		fprintf(stderr, "open file %s: %s\n", filename, err_mess);
		return -1;
	}
	fseek(file, 0L, SEEK_END);
	long sz = ftell(file);
	rewind(file);
	return sz;
}

int print_error(char *mess){
	char err_mess[256];
	int errnum = errno;
	strerror_r(errnum, err_mess, sizeof(err_mess));
	fprintf(stderr, "%s [ERROR]: %s\n", mess, err_mess);
	return errnum;
}
