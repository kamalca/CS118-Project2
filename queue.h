#ifndef QUEUE
#define QUEUE

#include packet.h

struct node {
	struct packet* p;
	node* next;
}

typedef struct queue {
	node* head;
	node* tail;
	int len;
} Queue;

void init(Queue* q){
	q->head = NULL;
	q->tail = NULL;
	q->len = 0;
}

void push(Queue* q, struct packet* p){
	node n;
	n->packet = p;
	n->next = NULL;
	if(q->len == 0){
		q->head = n;
		q->tail = n;
	}
	else{
		q->tail->next = n;
		q->tail = n;
	}
	q->len++;
}

struct packet* pop(Queue* q){
	struct packet* p = q->head->packet;
	q->head = q->head->next;
	q->len--;
	return p;
}