#ifndef _DATA_STRUCTURE_
#define _DATA_STRUCTURE_

#include "peer/Segment.h"

#define INSERT_NODE_SUCCESS 1
#define INSERT_NODE_NOT_IN_LL -2 
#define INSERT_NODE_ARGS_NULL -1

#define REMOVE_NODE_ARGS_NULL -1
#define REMOVE_NODE_EMPTY_LL -2
#define REMOVE_NODE_SUCCESS 1

#define SEGMENT_TYPE	0
#define INT_TYPE		1 

struct Node{
	void *data;
	struct Node *next;
	struct Node *prev;
};

struct LinkedList{
	struct Node *head;
	struct Node *tail;
	unsigned int n_nodes;
};

struct Node* newNode(void *data, int data_type);

struct LinkedList* newLinkedList();

void destructLinkedList(struct LinkedList *ll);

int llistContain(struct LinkedList ll, struct Node *node);

int insertNode(struct LinkedList *ll, struct Node *node, struct Node *offset);

int removeNode(struct LinkedList *ll, struct Node *node);

struct Node* pop(struct LinkedList *ll);

void push(struct LinkedList *ll, struct Node *node);

#endif
