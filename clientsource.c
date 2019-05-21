#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "header.h"

int transmit(int file){
  //this function will be used to split the file into smaller sections and send to server
  return -1;
}

int handshake(struct sockaddr* address){
  //send syn
  //wait for ack
  return -1;
}

int iptoint(char* host){
  return -1;
}

int stringtoint(char* string){
  int i, size = strlen(string), number = 0;
  for (i = 0; i < size; i++){
    if (string[i] > '9' || string[i] < '0')
      return -1;
    number += string[i] - '0';
    number *= 10;
  }
  return number/10;
}

int main(int argc, char* argv[]){

  //handling command line arguments
  int servername;
  int port;
  char* filename;

  if (argc != 4){
    fprintf(stderr, "Incorrect number of arguments.\nclient should be invoked with three arguments as\n./client <HOSTNAME-OR-IP> <PORT> <FILENAME\n");
    exit(1);
  }

  if (strcmp(argv[1], "localhost") == 0)
    servername = 0x7f000001;
  else{
    servername = iptoint(argv[1]);
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
    fprintf(stderr, "Couldn't open file %s\n", filename);
    exit(1);
  }

  //setting up socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  socklen_t size = sizeof(struct sockaddr_in);
  struct sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  their_addr.sin_addr.s_addr = htonl(servername);
  memset(&their_addr, 0, sizeof(their_addr));
  struct sockaddr* addr = (struct sockaddr*) &their_addr;

  //do handshake
  if (handshake(addr) == -1){
    fprintf(stderr, "handshake failed\n");
    close(sockfd);
    exit(1);
  }

  //split up file, send in packets
  if (transmit(filefd) == -1){
    fprintf(stderr, "transmission failed\n");
    close(sockfd);
    exit(1);
  }

  //closing socket
  close(sockfd);
  close(filefd);

  return 0;
}
