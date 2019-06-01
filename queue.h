#ifndef QUEUE
#define QUEUE

#include "packet.h"
#include <sys/time.h>
#include <stdlib.h>

struct node {
	struct packet* p;
	struct timeval tv;
	struct node* next;
};

typedef struct queue {
	struct node* head;
	struct node* tail;
	int len;
} Queue;

void init(Queue* q){
	q->head = NULL;
	q->tail = NULL;
	q->len = 0;
}

void push(Queue* q, struct packet* p){
	struct node n;
	n.p = p;
	gettimeofday(&(n.tv), 0);
	n.next = NULL;
	if(q->len == 0){
		q->head = &n;
		q->tail = &n;
	}
	else{
		q->tail->next = &n;
		q->tail = &n;
	}
	q->len++;
}

struct packet* pop(Queue* q){
	if(q->head == NULL){
		return NULL;
	}
	struct packet* p = q->head->p;
	q->head = q->head->next;
	q->len--;
	return p;
}

void delete(Queue* q){
	struct packet* p;
	while((p = pop(q))){
		free(p);
		p = NULL;
	}
	init(q);
}

struct timeval getTimer(Queue* q){
	struct timeval now;
	struct timeval diff;
	gettimeofday(&now, 0);
	timersub(&(q->head->tv), &now, &diff);
	return diff;
}

unsigned short getSeq(Queue* q){
	return q->head->p->seqNum;
}

void resetTimer(Queue* q){
	gettimeofday(&(q->head->tv), 0);
}

#endif
