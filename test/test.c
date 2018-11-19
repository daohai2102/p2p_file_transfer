#include <stdio.h>
#include "../src/LinkedList.h"

int main(){
	struct LinkedList *ll = newLinkedList();
	struct Node *it = ll->head;
	int i = 0;
	struct Node *tmp = newNode(99);
	insertNode(ll, tmp, NULL);

	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", it->data);
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", it->data);
	}
	printf("\n");
	for (; i < 10; i++){
		struct Node *node = newNode(i);
		push(ll, node);
	}
	
	tmp = newNode(100);
	insertNode(ll, tmp, NULL);

	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", it->data);
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", it->data);
	}
	printf("\n");

	tmp = ll->head->next->next;
	removeNode(ll, tmp);
	for (it = ll->head; it != NULL; it = it->next){
		printf("%d ", it->data);
	}
	printf("\n");

	for (it = ll->tail; it != NULL; it = it->prev){
		printf("%d ", it->data);
	}
	printf("\n");
}
