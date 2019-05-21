#ifndef PACKET
#define PACKET


struct packet {
	unsigned short seqNum;
	unsigned short ackNum;
	char ack;
	char syn;
	char fin;
	char zeros[5];
	char message[512];
};


#endif
