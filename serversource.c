#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "packet.h"

#define BACKLOG 10

int main(int argc, char* argv[]){
	if(argc != 2){
		fprintf(stderr, "usage: %s [port number]\n", argv[0]);
		exit(1);
	}
	int port = strtol(argv[1], (char**) NULL, 10);
	if(port < 0 || port > 65535){
		fprintf(stderr, "Invalid port number.\n");
		fprintf(stderr, "usage: %s [port number]\n", argv[0]);
		exit(1);
	}

	int sockfd, new_fd;
  socklen_t size = sizeof(struct sockaddr_in);
  struct sockaddr_in my_addr, their_addr;
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sockfd == -1){
    fprintf(stderr, "Error: Could not create socket\n");
    exit(1);
  }

//allows sockfd to be reused
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) == -1){
    fprintf(stderr, "setsockopt error number %i\n", errno);
    exit(1);
  }

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

  if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr)) == -1){
    fprintf(stderr, "Could not bind socket\n");
    exit(1);
  }


  struct packet* sendingPacket = calloc(1, sizeof(struct packet));
  struct packet* receivedPacket = calloc(1, sizeof(struct packet));
  struct sockaddr_in* cliaddr = calloc(1, sizeof(struct sockaddr_in));
  while (1){
		//main loop waits for requests from clients and then serves them
  	memset(sendingPacket, 0, sizeof(struct packet));
  	memset(receivedPacket, 0, sizeof(struct packet));
  	memset(cliaddr, 0, sizeof(struct sockaddr));

  	sendingPacket->seqNum = 123;
	sendingPacket->syn = 1;
	strcpy(sendingPacket->message, "Testing message.\n");

    unsigned int len, n; 
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, (char *)receivedPacket, 524, MSG_WAITALL,
    	(struct sockaddr *) cliaddr, (socklen_t *) &len); 
    printf("Client: %s\n", receivedPacket->message); 
    n = sendto(sockfd, sendingPacket->message, 512,  
        0, (const struct sockaddr *) cliaddr, 
            (socklen_t) sizeof(struct sockaddr_in)); 

  }
  free(sendingPacket);
  free(receivedPacket);
  free(cliaddr);
}










