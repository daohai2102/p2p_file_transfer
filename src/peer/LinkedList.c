#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

struct Node* newNode(struct Segment seg){
	struct Node* newN = malloc(sizeof(struct Node));
	newN->seg = seg;
	return newN;
}

struct LinkedList* newLinkedList(){
	struct LinkedList *ll = malloc(sizeof(struct LinkedList));
	ll->head = NULL;
	ll->tail = NULL;
	ll->n_nodes = 0;
	return ll;
}

void destructLinkedList(struct LinkedList *ll){
	if (ll->n_nodes == 0){
		free(ll);
		return;
	}

	struct Node *it = ll->head;

	while (it != NULL){
		struct Node *tmp = it->next;
		free(it);
		it = tmp;
	}

	free(ll);
}

int llistContain(struct LinkedList ll, struct Node *node){
	if (ll.n_nodes == 0){
		return 0;
	}

	struct Node *it = ll.head;
	for ( ; it != NULL; it = it->next){
		if (it == node)
			return 1;
	}

	return 0;
}

int insertNode(struct LinkedList *ll, struct Node *node, struct Node *offset){
	if (!ll || !node){
		fprintf(stderr, "insertNode: both arguments 'll' and 'node' must not be NULL\n");
		return INSERT_NODE_ARGS_NULL;
	}
	
	if (ll->n_nodes == 0){
		ll->n_nodes ++;
		ll->head = node;
		ll->head->prev = NULL;
		ll->head->next = NULL;
		ll->tail = node;
		return INSERT_NODE_SUCCESS;
	}

	if (!llistContain(*ll, offset) && offset != NULL){
		fprintf(stderr, "insertNode: offset is not a node in the linkedlist\n");
		return INSERT_NODE_NOT_IN_LL;
	}

	if (offset == NULL){
		//insert into the head
		struct Node *tmp = ll->head;
		ll->head = node;
		ll->head->next = tmp;
		ll->head->next->prev = ll->head;
	} else {
		struct Node *tmp = offset->next;
		offset->next = node;
		node->next = tmp;
		node->prev = offset;

		if (offset == ll->tail){
			ll->tail = node;
		} else {
			node->next->prev = node;
		}
	}
	ll->n_nodes += 1;
	return INSERT_NODE_SUCCESS;
}

int removeNode(struct LinkedList *ll, struct Node *node){
	if (!ll || !node){
		fprintf(stderr, "both arguments must not be NULL\n");
		return REMOVE_NODE_ARGS_NULL;
	}

	if (ll->n_nodes <= 0){
		fprintf(stderr, "the list is empty\n");
		return REMOVE_NODE_EMPTY_LL;
	}

	struct Node *prev = node->prev;
	struct Node *next = node->next;

	if (ll->n_nodes == 1){
		ll->head = NULL;
		ll->tail = NULL;
	} else if (ll->head == node){
		//remove the head
		ll->head = node->next;
		ll->head->prev = NULL;
	} else if (ll->tail == node){
		//remove the tail
		ll->tail = node->prev;
		ll->tail->next = NULL;
	} else {
		prev->next = next;
		next->prev = prev;
	}

	free(node);
	ll->n_nodes --;
	prev->next = next;
	next->prev = prev;

	return REMOVE_NODE_SUCCESS;
}

void push(struct LinkedList *ll, struct Node *node){
	insertNode(ll, node, ll->tail);
}

struct Node* pop(struct LinkedList *ll){
	if (ll->n_nodes == 0)
		return NULL;
	struct Node *res = ll->head;
	removeNode(ll, ll->head);
	return res;
}