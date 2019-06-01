#ifndef QUEUE
#define QUEUE

#include "packet.h"
#include <time.h>

struct node {
	struct packet* p;
	struct timeval tv;
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
	gettimeofday(&(n->tv), 0);
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
	if(q->head == NULL){
		return NULL;
	}
	struct packet* p = q->head->packet;
	q->head = q->head->next;
	q->len--;
	return p;
}

void delete(Queue* q){
	struct packet p;
	while((p = pop(q))){
		free(p);
		p = NULL;
	}
}

struct timeval getTimer(Queue* q){
	struct timeval now;
	struct timeval diff;
	gettimeofday(&now, 0);
	timersub(&(q->head->tv), &now, &diff);
	return diff;
}

