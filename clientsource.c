#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include "packet.h"
#include <errno.h>

int transmit(int file){
  //this function will be used to split the file into smaller sections and send to server
  printf("%i\n", file);
  return -1;
}

int handshake(int sockfd, struct sockaddr* address){
  struct packet syn = {0, 0, 0, 1, 0, {0, 0, 0, 0, 0}, {}}, synack;
  socklen_t size = sizeof(struct sockaddr_in);
  //send syn
  if (sendto(sockfd, (void*) &syn, sizeof(syn), 0, address, size) == -1){
    fprintf(stderr, "Couldn't send syn, errno %i\n", errno);
    return -1;
  }

  //wait for ack
  if (recvfrom(sockfd, (void*) &synack, sizeof(synack), 0, NULL, 0) == -1){
    fprintf(stderr, "Couldn't receive synack, errno %i\n", errno);
    return -1;
  }
  if (synack.syn != 1)
    return -1;
  return 0;
}

int iptoint(char* host){
  printf("%s\n", host);
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
    fprintf(stderr, "Incorrect number of arguments.\nclient should be invoked with three arguments as\n./client <HOSTNAME-OR-IP> <PORT> <FILENAME>\n");
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
  struct sockaddr_in their_addr;
  their_addr.sin_family = AF_INET;
  their_addr.sin_port = htons(port);
  their_addr.sin_addr.s_addr = htonl(0x7f000001);
  memset(their_addr.sin_zero, '\0', sizeof(their_addr.sin_zero));
  struct sockaddr* addr = (struct sockaddr*) &their_addr;

  //do handshake
  if (handshake(sockfd, addr) == -1){
    fprintf(stderr, "Handshake failed\n");
    close(sockfd);
    exit(1);
  }

  //transmit file
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
