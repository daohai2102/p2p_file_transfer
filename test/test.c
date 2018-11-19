#include <stdio.h>
#include "../src/LinkedList.h"

int main(){
	struct LinkedList *ll = newLinkedList();
	struct Node *it = ll->head;
	int i = 0;
	int data = 99;
	struct Node *tmp = newNode(&data, INT_TYPE);
	insertNode(ll, tmp, NULL);

	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");
	for (; i < 10; i++){
		struct Node *node = newNode(&i, INT_TYPE);
		push(ll, node);
	}
	
	data = 100;
	tmp = newNode(&data, INT_TYPE);
	insertNode(ll, tmp, NULL);

	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");

	tmp = ll->head->next->next;
	removeNode(ll, tmp);
	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", *((int*)it->data));
	}
	printf("\n");
}
