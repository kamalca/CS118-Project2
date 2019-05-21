
struct header {
	unsigned short seqNum;
	unsigned short ackNum;
	char ack;
	char syn;
	char fin;
	char zeros[5];
}