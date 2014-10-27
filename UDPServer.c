
/*
** UDPserver.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#define MAXBUFLEN 10000

struct __attribute__((__packed__)) MessageFrom 
{
	uint16_t msgLength;
	char checksum;
	uint8_t gid;
	uint8_t reqID;
	uint8_t delimiter;
	char host[10000];
};

struct __attribute__((__packed__)) MessageToValid
{
	uint16_t msgLength;
	uint8_t checksum;
	uint8_t gid;
	uint8_t reqID;
	uint8_t ip[3000];
};

//headers for valid and length mismatch????
struct __attribute__((__packed__)) MessageToInvalid
{
	char checksum;
	uint8_t gid;
	uint8_t reqID;
	uint8_t	byte1;//set to 0
	uint8_t byte2;//set to 0
};

struct __attribute__((__packed__)) MessageToMismatch
{
	char checksum;
	uint8_t gid; //set to 127
	uint8_t reqID;//set to 127
	uint8_t	byte1;//set to 0
	uint8_t byte2;//set to 0
};


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr,"usage: server PortNumber\n");
		exit(1);
	}
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	int i = 0;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	socklen_t sin_size;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; //use my IP

	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	printf("server: waiting to recvfrom...\n");

	while(1) {

		sin_size = sizeof their_addr;
		


		if (!fork()){ // this is the child process
			struct MessageFrom newMessageFrom;
			//struct MessageToValid newMessageToValid;

			addr_len = sizeof their_addr;

			if ((numbytes = recvfrom(sockfd, (char *) &newMessageFrom, MAXBUFLEN-1 , 0,
				(struct sockaddr *)&their_addr, &addr_len)) == -1) {
					perror("recvfrom");
					exit(1);
			}
			
			
			// Calculate the checksum  
			int chk;
			char checksum;
			int carry;
			chk = 0;
			carry = 0;
			
			newMessageFrom.msgLength = ntohs(newMessageFrom.msgLength);
			char* bytesFrom = (char*)&newMessageFrom.msgLength;
			
			chk = bytesFrom[0] & 0x00FF;
			carry = chk >> 8;
			chk = chk + carry;
			
			
			chk = chk + (bytesFrom[1] & 0x00FF);
			carry = chk >> 8;
			chk = (chk & 0xFF) + carry;
			
			
			chk = chk + (newMessageFrom.gid & 0x00FF);
			carry = chk >> 8;
			chk = (chk & 0xFF) + carry;
			
			
			chk = chk + (newMessageFrom.reqID & 0x00FF);
			carry = chk >> 8;
			chk = (chk & 0xFF) + carry;
			
			
			chk = chk + (newMessageFrom.delimiter & 0x00FF);
			carry = chk >> 8;
			chk = (chk & 0xFF) + carry;
			
			
			int nextByte;
			for(nextByte = 0; nextByte < strlen(newMessageFrom.host); nextByte++)
			{
				
				chk = chk + (newMessageFrom.host[nextByte]& 0x00FF);
				carry = chk >> 8;
				chk = (chk & 0xFF) + carry;
			
			}
			
			
			
			chk = chk + (newMessageFrom.checksum & 0x00FF);
			carry = chk >> 8;
			chk = (chk & 0xFF) + carry;
			
			checksum = (char)chk;
			
			
			//check checksum for correctness
				if (checksum != -1)
				{
					printf("Checksum error!\n");
					printf("%d\n", checksum);
					struct MessageToInvalid newMessageToInvalid;
					int chkerr = 0;
					char carryerr = 0;
					newMessageToInvalid.gid = newMessageFrom.gid;
					chkerr = newMessageToInvalid.gid & 0x00FF;
					carryerr = chkerr >> 8;
					chkerr = (chkerr & 0xFF) + carryerr;
					newMessageToInvalid.reqID = newMessageFrom.reqID;
					chkerr = (newMessageToInvalid.reqID & 0x00FF) + chkerr;
					carryerr = chkerr >> 8;
					chkerr = (chkerr & 0xFF) + carryerr;
					newMessageToInvalid.byte1 = 0;
					newMessageToInvalid.byte2 = 0;
					newMessageToInvalid.checksum = ~chkerr;
					if ((numbytes = sendto(sockfd, (char *) &newMessageToInvalid, 5, 0,
						(struct sockaddr *)&their_addr, addr_len)) == -1)
					{
							perror("server: sendto");
							exit(1);
					}
				}
				else
				{
					//printf("Checksum Correct!\n");
				}
				if (newMessageFrom.msgLength != (strlen(newMessageFrom.host) + 6))
				{
					printf("Length Mismatch!\n");
					struct MessageToMismatch newMessageToMismatch;
					int chklen = 0;
					char carrylen = 0;
					newMessageToMismatch.gid = 127;
					chklen = newMessageToMismatch.gid & 0x00FF;
					carrylen = chklen >> 8;
					chklen = (chklen & 0xFF) + carrylen;
					newMessageToMismatch.reqID = 127;
					chklen = (newMessageToMismatch.reqID & 0x00FF) + chklen;
					carrylen = chklen >> 8;
					chklen = (chklen & 0xFF) + carrylen;
					newMessageToMismatch.byte1 = 0;
					newMessageToMismatch.byte2 = 0;
					newMessageToMismatch.checksum = ~chklen;
					if ((numbytes = sendto(sockfd, (char *) &newMessageToMismatch, 5, 0,
						(struct sockaddr *)&their_addr, addr_len)) == -1)
					{
							perror("server: sendto");
							exit(1);
					}
				}
				


			//Begin getting the hostname
			int i;
			struct hostent *he;
			struct in_addr **addr_list;
			struct MessageToValid newMessageToValid;
			char *token[5000], *IPtoken[5000];
		
			newMessageFrom.host[newMessageFrom.msgLength-6] = '\0';
			
			//get delimiter char for use in method
			char del = newMessageFrom.delimiter;
			char delim[1];
			delim[0] = del;
			token[0] = strtok(newMessageFrom.host, delim);
			int incrementIPX = 0, incrementIPY = 0;

			i = 0;
			while(token[i]!= NULL) {  
				i++;
				token[i] = strtok(NULL, delim); //tokenize the string
			}
			uint8_t ip[1000][1000];
			//token array now has each hostname
			int j, k, structPosition;
			k = 0;
			structPosition = 0;
			char tempCharByte[4] = {0};
			uint8_t tempByte = 0;
			uint8_t tempip[5000];
			int numBytesForLength = 0;
			int numberOfIPsArray[500];
			for(j = 0; j < i; j++) {
				
				if ((he = gethostbyname(token[j])) == NULL) {  // get the host info
					char invalidIP[] = "255.255.255.255";
					IPtoken[0] = strtok(invalidIP, ".");

					int m = 0;
					while(IPtoken[m]!= NULL) {  
						m++;
						IPtoken[m] = strtok(NULL, "."); //tokenize the string
					}
					incrementIPY = 0;
					ip[incrementIPX][incrementIPY] = 1;
					incrementIPY++;
					numBytesForLength++;
					for(k = 0; k < m; k++) {
						int cast = (atoi(IPtoken[k]));
						ip[incrementIPX][incrementIPY]  = (uint8_t)(cast&0x000000FF);
						incrementIPY++;
						numBytesForLength++;
					}
					incrementIPX++;
					}
				else {
					addr_list = (struct in_addr **)he->h_addr_list;
					int numIPs, numIPsCount;
					incrementIPY = 0;
					int numberOfIPAddresses = 0;
					for(numIPsCount = 0; addr_list[numIPsCount] != NULL; numIPsCount++)
					{
						numberOfIPAddresses++;
					}
					ip[incrementIPX][incrementIPY] = numberOfIPAddresses;
					
					incrementIPY++;
					numBytesForLength++;
					for(numIPs = 0; addr_list[numIPs] != NULL; numIPs++)
					{
					
						IPtoken[0] = strtok(inet_ntoa(*addr_list[numIPs]), ".");
						int m = 0;
						while(IPtoken[m]!= NULL) {  
							m++;
							IPtoken[m] = strtok(NULL, "."); //tokenize the string
						}
						for(k = 0; k < m; k++) {
							int cast = (atoi(IPtoken[k]));
							/*modified, cast to unsigned 8-bit integer*/
							ip[incrementIPX][incrementIPY]  = (uint8_t)(cast&0x000000FF);
							
							incrementIPY++;
							numBytesForLength++;
						}
					
					}	
					incrementIPX++;			
				}
				numberOfIPsArray[j] = incrementIPY;
			}	
			
			newMessageToValid.msgLength = ((numBytesForLength)+5);
			newMessageToValid.gid = newMessageFrom.gid;
			newMessageToValid.reqID = newMessageFrom.reqID;
			
			
			//Checksum
			uint8_t sendchk;
			sendchk = 0;
			carry = 0;
			
			uint16_t temp;
			temp = 0;

			int nextByteX, nextByteY;
			
			int OneDArrayPosition = 0;
			for(nextByteX = 0; nextByteX < incrementIPX; nextByteX++)
			{
				for(nextByteY = 0; nextByteY < numberOfIPsArray[nextByteX]; nextByteY++)
				{
					chk = chk + (ip[nextByteX][nextByteY] & 0x00FF);
					carry = chk >> 8;
					chk = (chk & 0xFF) + carry;
					newMessageToValid.ip[OneDArrayPosition] = ip[nextByteX][nextByteY];
					OneDArrayPosition++;
					
				}
			}
			char* bytes = (char*)&newMessageToValid.msgLength;
			
			for(i = 0; i < newMessageToValid.msgLength; i++)
			{ 
				temp += (uint8_t)*bytes;
				if(temp > 255)
				{
					temp += 1;
					temp &= 0xFF;
				}
				bytes++;
			}
			
			sendchk = ~temp;
		    /*     temp += sendchk;
                        if(temp > 255)
                        {
                                temp += 1;
                                temp &= 0xFF;
                        }
                        printf("current byte %d temp %d\n", sendchk, temp); */


		
			newMessageToValid.checksum = sendchk;
			
			newMessageToValid.msgLength = htons(newMessageToValid.msgLength);
			
			if ((numbytes = sendto(sockfd, (char *) &newMessageToValid,ntohs(newMessageToValid.msgLength), 0,
						(struct sockaddr *)&their_addr, addr_len)) == -1)
			{
							perror("server: sendto");
							exit(1);
			}

			

		}

	}
	exit(0);
	return 0;
}
