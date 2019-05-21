#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

void transmit(){

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
  char* servername;
  int port;
  char* filename;

  if (argc != 4){
    fprintf(stderr, "Incorrect number of arguments.\nclient should be invoked with three arguments as\n./client <HOSTNAME-OR-IP> <PORT> <FILENAME\n");
    exit(1);
  }
  servername = argv[1];
  port = stringtoint(argv[2]);
  if (port < 0 || port > 65535){
    fprintf(stderr, "Improper port number\n");
    exit(1);
  }
  filename = argv[3];

  //setting up socket
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  //socklen_t size = sizeof(struct sockaddr_in);
  //struct sockaddr_in their_addr;

  //opening target file
  int filefd = open(filename, O_RDONLY);
  if (filefd < 0)
    exit(1);

  //do handshake
  //split up file, send in packets
  //closing socket
  close(sockfd);

  return 0;
}
