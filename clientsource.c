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

int transmit(int file, int sockfd, struct sockaddr* address, unsigned short* seqNum, unsigned short* ackNum){
    //this function will be used to split the file into smaller sections and send to server
    int numRead, offset = 0;
    struct packet message, ack;

    do{//use alarm() function with signal handler for SIGALRM, also hinted at using C++ std::chrono
        //or use select() on the socket stream (but that would only work on stop-and-wait)
        //if numRead < payload, send with fin
        numRead = pread(file, message.message, PAYLOAD, offset);

        if (numRead == -1){
            fprintf(stderr, "Error reading file, %s\n", strerror(errno));
            return -1;
        }

        write(1, message.message, numRead);
        message.seqNum = *seqNum;
        message.ackNum = *ackNum;

        if(sendto(sockfd, (void*) &message, sizeof(message), 0, address, sizeof(*address)) == -1){
            fprintf(stderr, "Couldn't send packet to server, %s\n", strerror(errno));
            return -1;
        }
        printf("\nSending packet with seqNum %i and ackNum %i\n", *seqNum, *ackNum);

        //receive ack from server
        if (recvfrom(sockfd, (void*) &ack, sizeof(ack), 0, NULL, NULL) == -1){
            fprintf(stderr, "Couldn't receive ack from server, %i\n", errno);
            return -1;
        }
        printf("\nReceived packet with seqNum %i and ackNum %i\n", ack.seqNum, ack.ackNum);

        //check server ack
        (*seqNum) += numRead;
        if (ack.ackNum != *seqNum + 1){
            fprintf(stderr, "Wrong ack ackNum\n");
            return -1;
        }
        if (ack.seqNum != *ackNum){
            fprintf(stderr, "Wrong ack seqNum\n");
            return -1;
        }
        (*ackNum) += numRead;

        offset += numRead;
    }while (numRead == PAYLOAD);

    return 0;
}

int handshake(int sockfd, struct sockaddr* address, unsigned short* seqNum, unsigned short* ackNum){

    *seqNum = rand(); //assigning random sequence number

    struct packet syn = {*seqNum, 0, 0, 1, 0, {0, 0, 0, 0, 0}, {}}, synack;
    socklen_t size = sizeof(struct sockaddr_in);

    //send syn
    if (sendto(sockfd, (void*) &syn, sizeof(syn), 0, address, size) == -1){
        fprintf(stderr, "Couldn't send SYN to server, %s\n", strerror(errno));
        return -1;
    }
    printf("hand shaken with seqNum %i\n", *seqNum);

    //wait for ack
    if (recvfrom(sockfd, (void*) &synack, sizeof(synack), 0, NULL, 0) == -1){
        fprintf(stderr, "Couldn't receive SYNACK from server, %s\n", strerror(errno));
        return -1;
    }
    if (synack.syn != 1)
        return -1;
    printf("synack received with ackNum %i, server seqNum %i\n", synack.ackNum, synack.seqNum);
    *ackNum = synack.seqNum + 1;
    if (synack.ackNum != *seqNum + 1){
        fprintf(stderr, "synack returned incorrect ack number\n");
        return -1;
    }

    (*seqNum)++;
    printf("handshake complete, seqNum %i, ackNum %i\n", *seqNum, *ackNum);
    return 0;
}

long iptolong(char* ip){
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

    //closing socket
    close(sockfd);
    close(filefd);

    return 0;
}
