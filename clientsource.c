#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "packet.h"
#include <errno.h>
#define PAYLOAD 512



void printsent(struct packet* message, int cwnd, int ssthresh){
    fprintf(stderr, "SEND %i %i %i %i %i %i %i\n", message->seqNum, message->ackNum, cwnd, ssthresh, message->ack, message->syn, message->fin);
}



void printreceived(struct packet* message, int cwnd, int ssthresh){
    fprintf(stderr, "RECV %i %i %i %i %i %i %i\n", message->seqNum, message->ackNum, cwnd, ssthresh, message->ack, message->syn, message->fin);
}



//insert fin function here?



int transmit(int file, int sockfd, struct sockaddr* address, unsigned short* seqNum, unsigned short* ackNum){
    //this function will be used to split the file into smaller sections and send to server
    int numRead, offset = 0, cwnd = 512, ssthresh = 10240;
    struct packet message, ack;
    message.ack = 1;
    message.syn = message.fin = 0;
    memset(message.zeros, 0, sizeof(message.zeros));
    
    do{//use alarm() function with signal handler for SIGALRM, also hinted at using C++ std::chrono
        numRead = pread(file, message.message, PAYLOAD, offset);

        if (numRead == -1){
            fprintf(stderr, "Error reading file, %s\n", strerror(errno));
            return -1;
        }

        //write(1, message.message, numRead);//for debugging purposes
        message.seqNum = *seqNum;
        message.ackNum = *ackNum;

        if(sendto(sockfd, (void*) &message, 12 + numRead, 0, address, sizeof(*address)) == -1){
            fprintf(stderr, "Couldn't send packet to server, %s\n", strerror(errno));
            return -1;
        }
        printsent(&message, cwnd, ssthresh);

        //receive ack from server
        if (recvfrom(sockfd, (void*) &ack, 12, 0, NULL, NULL) == -1){
            fprintf(stderr, "Couldn't receive ack from server, %i\n", errno);
            return -1;
        }
        printreceived(&ack, cwnd, ssthresh);
        
        //check server ack
        (*seqNum) += numRead;
        if (ack.ackNum != *seqNum){
            fprintf(stderr, "Wrong ack ackNum, expected ackNum %i\n", *seqNum);
            return -1;
        } printf("seqNum = %i\n", *seqNum);
        if (ack.seqNum != *ackNum){
            fprintf(stderr, "Wrong ack seqNum, expected seqNum %i\n", *ackNum);
            return -1;
        }

        offset += numRead;
        message.ack = 0;
    }while (numRead == PAYLOAD);

    //use fin to close connection
    message.fin = 1;
    message.seqNum = *seqNum;
    if (sendto(sockfd, (void*) &message, 12, 0, address, sizeof(*address)) == -1){
        fprintf(stderr, "Couldn't send fin to server, %s\n", strerror(errno));
        return -1;
    } fprintf(stderr, "\n\nfin\n");
    printsent(&message, cwnd, ssthresh);
    
    //receive finack
    if (recvfrom(sockfd, (void*) &ack, 12, 0, NULL, NULL) == -1){
        fprintf(stderr, "Couldn't receive finack from server, %s\n", strerror(errno));
        return -1;
    }
    printreceived(&ack, cwnd, ssthresh);
    
    //recieve fin
    if (recvfrom(sockfd, (void*) &message, 12, 0, NULL, NULL) == -1){
        fprintf(stderr, "Couldn't receive fin from server, %s\n", strerror(errno));
        return -1;
    }
    printreceived(&message, cwnd, ssthresh);
    
    //send finack
    if (sendto(sockfd, (void*) &ack, 12, 0, address, sizeof(*address)) == -1){
        fprintf(stderr, "Couldn't send finack to server, %s\n", strerror(errno));
        return -1;
    }
    printsent(&ack, cwnd, ssthresh);
    fprintf(stderr, "fin complete\n\n\n");
    return 0;
}



int handshake(int sockfd, struct sockaddr* address, unsigned short* seqNum, unsigned short* ackNum){
    //creates handshake with server specified by address parameter

    *seqNum = rand() % 25600; //assigning random sequence number

    struct packet syn = {*seqNum, 0, 0, 1, 0, {0, 0, 0, 0, 0}, {}}, synack;
    socklen_t size = sizeof(struct sockaddr_in);

    //send syn
    if (sendto(sockfd, (void*) &syn, 12, 0, address, size) == -1){
        fprintf(stderr, "Couldn't send SYN to server, %s\n", strerror(errno));
        return -1;
    }
    fprintf(stderr, "handshake\n");
    printsent(&syn, 0, 0);

    //wait for ack
    if (recvfrom(sockfd, (void*) &synack, 12, 0, NULL, 0) == -1){
        fprintf(stderr, "Couldn't receive SYNACK from server, %s\n", strerror(errno));
        return -1;
    }
    if (synack.syn != 1)
        return -1;
    printreceived(&synack, 0, 0);
    *ackNum = synack.seqNum + 1;
    if (synack.ackNum != *seqNum + 1){
        fprintf(stderr, "synack returned incorrect ack number\n");
        return -1;
    }

    (*seqNum)++;
    fprintf(stderr, "handshake complete, seqNum = %i, ackNum = %i\n\n\n", *seqNum, *ackNum);
    return 0;
}



long iptolong(char* ip){
    //converts string ip in format x.x.x.x to a long integer
    int i = 0, a = 0;
    long result = 0;
    while (ip[i] != 0){
        a *= 10;
        if (ip[i] == '.'){
            result *= 256;
            result += a;
            a = 0;
        } else if (ip[i] < '0'|| ip[i] > '9'){
            fprintf(stderr, "improper ip address\n");
            return -1;
        } else
            a += ip[i] - '0';
        i++;
    }

    result *= 256;
    result += a;
    return result;
}



int stringtoint(char* string){
    //converts a string to the int it represents
    int i, size = strlen(string), number = 0;
    for (i = 0; i < size; i++){
        number *= 10;
        if (string[i] > '9' || string[i] < '0')
            return -1;
        number += string[i] - '0';
    }
    return number;
}



int main(int argc, char* argv[]){

    //handling command line arguments
    long servername;
    int port;
    char* filename;

    if (argc != 4){
        fprintf(stderr, "Incorrect number of arguments.\nclient should be invoked with three arguments as\n./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\n");
        exit(1);
    }

    if (strcmp(argv[1], "localhost") == 0)
        servername = 0x7f000001;
    else{
        servername = iptolong(argv[1]);
        if (servername == -1){
            fprintf(stderr, "Improper hostname\n");
            exit(1);
        }
    }

    port = stringtoint(argv[2]);
    if (port < 0 || port > 65535){
        fprintf(stderr, "Improper port number\n");
        exit(1);
    }

    filename = argv[3];
    int filefd = open(filename, O_RDONLY);
    if (filefd < 0){
        fprintf(stderr, "Couldn't open file %s, %s\n", filename, strerror(errno));
        exit(1);
    }

    //setting up socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    unsigned short seqNum, ackNum;
    struct sockaddr_in their_addr;
    their_addr.sin_family = AF_INET;
    their_addr.sin_port = htons(port);
    their_addr.sin_addr.s_addr = htonl(servername);
    memset(their_addr.sin_zero, '\0', sizeof(their_addr.sin_zero));
    struct sockaddr* addr = (struct sockaddr*) &their_addr;
    //timeout setting
    struct timeval timeout = {10, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void*) &timeout, sizeof(timeout));

    //create signal handler for SIGINT and SIGTERM, use globals

    //do handshake
    if (handshake(sockfd, addr, &seqNum, &ackNum) == -1){
        fprintf(stderr, "Handshake failed\n");
        close(sockfd);
        close(filefd);
        exit(1);
    }

    //transmit file
    if (transmit(filefd, sockfd, addr, &seqNum, &ackNum) == -1){
        fprintf(stderr, "Transmission failed\n");
        close(sockfd);
        close(filefd);
        exit(1);
    }
    
    //fin?

    //closing socket
    close(sockfd);
    close(filefd);

    return 0;
}
