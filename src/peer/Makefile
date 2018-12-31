# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -Wall -pthread

# the list of header files
#INCL   = server.h
#the list of source files
SRC = handle_download_file_request.c ../LinkedListUtils.c list_hosts_request.c list_files_request.c peer.c update_file_list.c download_file_request.c connect_index_server.c ../LinkedList.c ../common.c ../sockio.c
OBJ = $(SRC:.c=.o)

# the build target executable:
TARGET = peer

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

$(OBJ): $(INCL)

clean:
	$(RM) $(TARGET) $(OBJ)
