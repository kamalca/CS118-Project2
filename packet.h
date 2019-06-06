#ifndef PACKET
#define PACKET


struct packet {
	unsigned short seqNum;
	unsigned short ackNum;
	char ack;
	char syn;
	char fin;
	int len;
	char padding;
	char message[512];
};


#endif
