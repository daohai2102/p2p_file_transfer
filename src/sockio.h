#ifndef _SOCK_IO_H
#define _SOCK_IO_H

int readBytes(int sock, void* buf, unsigned int nbytes);
int writeBytes(int sock, void* buf, unsigned int nbytes);

#endif
