#ifndef _DATA_STRUCTURE_
#define _DATA_STRUCTURE_

#include "Segment.h"

#define INSERT_NODE_SUCCESS 1
#define INSERT_NODE_NOT_IN_LL -2 
#define INSERT_NODE_ARGS_NULL -1

#define REMOVE_NODE_ARGS_NULL -1
#define REMOVE_NODE_EMPTY_LL -2
#define REMOVE_NODE_SUCCESS 1

struct Node{
	struct Segment seg;
	struct Node *next;
	struct Node *prev;
};

struct LinkedList{
	struct Node *head;
	struct Node *tail;
	unsigned int n_nodes;
};

struct Node* newNode(struct Segment seg);

struct LinkedList* newLinkedList();

void destructLinkedList(struct LinkedList *ll);

int llistContain(struct LinkedList ll, struct Node *node);

int insertNode(struct LinkedList *ll, struct Node *node, struct Node *offset);

int removeNode(struct LinkedList *ll, struct Node *node);

struct Node* pop(struct LinkedList *ll);

void push(struct LinkedList *ll, struct Node *node);

#endif
