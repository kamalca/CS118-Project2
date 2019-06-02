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
#include <time.h>
#include <signal.h>
#include "packet.h"

#define BACKLOG 10
#define MAXSEQ 25600
#define RWND 20

//TESTING
const char* testMessage = "Message: \"";

char filename[50];

//TESTING
void printPacket(struct packet* p){
	printf("SEQ: %d\n", p->seqNum);
	printf("ACK: %d\n", p->ackNum);
	printf("Flags: ack %d, syn %d, fin %d\n", p->ack, p->syn, p->fin);
}

void signalReceived(int sig){
	if(sig == SIGQUIT || sig == SIGTERM) {
		write(2, "Stopping Server.\n", 17);
		if(strlen(filename) > 0){
			int outfd = open(filename, O_CREAT | O_WRONLY, S_IRWXU);
			if(outfd < 0){
				fprintf(stderr, "Could not open file: %s\n", filename);
				_exit(0);
			}
			if(ftruncate(outfd, 0) < 0){
				fprintf(stderr, "Could not clear file: %s\n", filename);
				fprintf(stderr, "Error: %s\n", strerror(errno));
				_exit(0);
			}
			write(outfd, "INTERRUPT\n", 10);
		}
		_exit(0);
	}
}

void serveClient(int sockfd, int connectionNum){
	srand(time(NULL));
	struct packet* sendingPacket = calloc(1, sizeof(struct packet));
	struct packet* receivedPacket = calloc(1, sizeof(struct packet));
	struct sockaddr_in* cliaddr = calloc(1, sizeof(struct sockaddr_in));
    char* buff[RWND];
    int buffLen[RWND];
    memset(buff, 0, RWND*sizeof(buff[0]));
    memset(buffLen, 0, RWND*sizeof(buffLen[0]));
	int outfd = -1;
	int len, n;
	int window = -1;
	int seq = rand() % MAXSEQ;
	int fin = 0;

	
	//Testing
	// strcpy(sendingPacket->message, "Testing message.\n");
	// sendto(sockfd, sendingPacket, 524,  0,
	// 	(const struct sockaddr *) cliaddr,
	// 	(socklen_t) sizeof(struct sockaddr_in)); 

	while(!fin){
		memset(sendingPacket, 0, sizeof(struct packet));
		memset(receivedPacket, 0, sizeof(struct packet));
		memset(cliaddr, 0, sizeof(struct sockaddr));

		len = sizeof(cliaddr);
		n = recvfrom(sockfd, (char *)receivedPacket, sizeof(struct packet), MSG_WAITALL,
				(struct sockaddr *) cliaddr, (socklen_t *) &len);
		if(n < 1){
			fprintf(stderr, "Timeout\n");
			if(window == -1 || outfd < 0){
				continue;
			}
			else{
				filename[0] = 0;
				close(outfd);
				outfd = -1;
				break;
			}
		}

		//Testing
		printf("\nReceived:\n");
		printPacket(receivedPacket);

		//Received SYN packet
		if(receivedPacket->syn != 0){
			snprintf(filename, 49, "%d.file", connectionNum);
			outfd = open(filename, O_CREAT | O_WRONLY, S_IRWXU);
			if(outfd < 0){
				fprintf(stderr, "Could not open file: %s\n", filename);
				break;
			}
			if(ftruncate(outfd, 0) < 0){
				fprintf(stderr, "Could not clear file: %s\n", filename);
				fprintf(stderr, "Error: %s\n", strerror(errno));
				close(outfd);
				outfd = -1;
				break;
			}

			sendingPacket->syn = 1;
			window = receivedPacket->seqNum + 1;
		}

		//First packet is not SYN
		if(window == -1 || outfd < 0){
			fprintf(stderr, "First Packet is not SYN. (Dropped)\n");
			continue;
		}

		//Packet in order
		if(receivedPacket->seqNum == window){
			//Inconsistency Check
			if(receivedPacket->ack && receivedPacket->ackNum != seq){
				fprintf(stderr, "Packet ACK may not be correct.\n");
			}

			//Write payload to file and increase window
			write(0, testMessage, strlen(testMessage));
			write(0, receivedPacket->message, n-12);
			if(write(outfd, receivedPacket->message, n-12) < 0){
				fprintf(stderr, "Could not print to fd %d\n", outfd);
				fprintf(stderr, "Error: %s\n", strerror(errno));
			}
			printf("\"\n");
			window = (receivedPacket->seqNum + n - 12) % (MAXSEQ+1);

			//Write any (sequential) buffered packets to file
			int i;
			for(i = 0; buff[i] != NULL && i < RWND; i++){
				write(0, testMessage, strlen(testMessage));
				write(0, buff[i], buffLen[i]);
				write(outfd, buff[i], buffLen[i]);
				printf("\"\n");
				window += (buffLen[i]) % (MAXSEQ+1);
				free(buff[i]);
				buff[i] = NULL;
				buffLen[i] = 0;
			}
			//Move remaining (non-sequential) buffered packets forward
			for(int j = i + 1; j < RWND; j++){
				if(buff[j] != NULL){
					buff[j-i-1] = buff[j];
					buffLen[j-i-1] = buffLen[j];
					buff[j] = NULL;
					buffLen[j] = 0;
				}
			}

			//Packet was FIN
			if((fin = receivedPacket->fin)){
				window = (window + 1) % (MAXSEQ+1);
				filename[0] = 0;
				close(outfd);
				outfd = -1;
			}
		}
		//Packet out of order
		else if(receivedPacket->syn == 0 && receivedPacket->fin == 0){
			int i;

			//Calculate buffer location. Must handle wrap-around for SeqNum
			if(receivedPacket->seqNum > window)
				i = ((receivedPacket->seqNum - window) / 512) - 1;
			else
				i = (((receivedPacket->seqNum + MAXSEQ + 1) - window) / 512) - 1;

			if(buff[i] == NULL){
				buff[i] = calloc(1, n - 12);
				strncpy(buff[i], receivedPacket->message, n - 12);
				buffLen[i] = n - 12;
			}
		}

		//ACK the next expected packet
		sendingPacket->seqNum = seq;
		sendingPacket->ackNum = window;
		sendingPacket->ack = 1;
		if(sendingPacket->syn != 0)
			seq++;

		//Testing
		printf("\nWindow: %d\n", window);
		printf("\nSending:\n");
		printPacket(sendingPacket);

		//Send ACK
		sendto(sockfd, sendingPacket, 12,  
			0, (const struct sockaddr *) cliaddr, 
			(socklen_t) sizeof(struct sockaddr_in));
	}

	if(fin){
		memset(sendingPacket, 0, sizeof(struct packet));
		sendingPacket->fin = 1;
		sendingPacket->seqNum = seq;

		//Testing
		printf("\nSending:\n");
		printPacket(sendingPacket);

		//Send FIN
		sendto(sockfd, sendingPacket, 12,  
			0, (const struct sockaddr *) cliaddr, 
			(socklen_t) sizeof(struct sockaddr_in));

		len = sizeof(cliaddr);
		n = recvfrom(sockfd, (char *)receivedPacket, sizeof(struct packet), MSG_WAITALL,
				(struct sockaddr *) cliaddr, (socklen_t *) &len);
		printf("\nReceived:\n");
		printPacket(receivedPacket);
		if(n == 12 && receivedPacket->ack == 1){
			printf("Connection Closed Successfully\n");
		}
		else{
			printf("Connection not propperly closed\n");
		}
	}


	free(sendingPacket);
	free(receivedPacket);
	free(cliaddr);
	close(outfd);
}

int main(int argc, char* argv[]){
	signal(SIGTERM, signalReceived);
	signal(SIGQUIT, signalReceived);
	signal(SIGINT, signalReceived);
	filename[0] = 0;

	if(argc != 2){
		fprintf(stderr, "ERROR: usage: %s [port number]\n", argv[0]);
		exit(1);
	}
	int port = strtol(argv[1], (char**) NULL, 10);
	if(port < 0 || port > 65535){
		fprintf(stderr, "ERROR: Invalid port number.\n");
		exit(1);
	}

	int sockfd;
	struct sockaddr_in my_addr;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd == -1){
	fprintf(stderr, "ERROR: Could not create socket\n");
	exit(1);
	}

	//allows sockfd to be reused
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0){
		fprintf(stderr, "ERROR setsockopt: %s\n", strerror(errno));
		exit(1);
	}
	//Sets timeout for receiving new data
	struct timeval timeOut;
	timeOut.tv_sec = 10;
	timeOut.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut)) < 0) {
	    fprintf(stderr, "ERROR setsockopt: %s\n", strerror(errno));
		exit(1);
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(my_addr.sin_zero, '\0', sizeof(my_addr.sin_zero));

	if (bind(sockfd, (struct sockaddr*) &my_addr, sizeof(struct sockaddr)) == -1){
	fprintf(stderr, "ERROR: Could not bind socket\n");
	exit(1);
	}

	for(int i = 1 ;; i++){
	//Serve Clients
		serveClient(sockfd, i);
	}

	}
