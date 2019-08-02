/* 
Written by Zach Grimes 
*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


void error(char *msg)
{
	perror(msg);
	exit(1);
}

/* Converts a single digit base 8 number into a bipolar base 2 number */
int* BipolBinaryConv(int val) {
	int* bbits = (int*) malloc(sizeof(int) * 3);
	int quot=val;
	for(int i=2; i>=0; i--) {
		if( (quot - (2*(quot/2))) )
			bbits[i] = 1;
		else
			bbits[i] = -1;
		quot/=2;
	}
	return bbits;
}
/*
 Call to encoder three times, each time passing a base 8 number-
 and it's corresponding Walsh code, converting it to bipolar binary.
 
 Each EM[p] value is calculated as follows:
 EM[1...p-1] = Bipolar Binary digit[0...3] * w[1...p-1]
 where p is the size of the Hadamard matrix
 
 Final EM is summed in the main function due to scope of assignment.
 */
int* Encoder(int* code, int val)
{
	int* signal = (int*) malloc(sizeof(int) * 12);
	int* bbinVal = BipolBinaryConv(val);
	for(int i =0; i<3; i++)
		for(int j=(i*4); j < ((i+1)*4); j++)
			signal[j] = (bbinVal[i]*code[(j-(i*4))]);

	return signal;
}

/* Begin server set up*/
int main(int argc, char *argv[])
{
	int sockfd, n, snum=0, portno, clilen;
	char ReadBuffers[3][3];
	int WriteBuffers[3][16];
	int newsockfd[3];
	int codes[3][4]= {{-1,1,-1,1},{-1,-1,1,1},{-1,1,1,-1}};
	int em[12];
	struct sockaddr_in serv_addr, cli_addr;
	if (argc < 2) {
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		error("ERROR on binding");
	clilen = sizeof(cli_addr);
	
	while(snum < 3) {
		listen(sockfd,5);
		newsockfd[snum] = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
		if (newsockfd < 0)
			error("ERROR: Child process could not be accepted");
		else
			snum++;
	}
	
	char tempbuffer[3];
	for(int i=0; i < 3; i++) {
		n = read(newsockfd[i],tempbuffer,3); //uses the buffer to read from sender/receiver program
		printf("Here is the message from child %c: , Value = %c, Destination = %c\n", tempbuffer[0],tempbuffer[2],tempbuffer[1]);
		for(int j=0; j<3;j++)
			ReadBuffers[i][j] = tempbuffer[j];
		bzero(tempbuffer,3);
	}

	if (n<0)
		error("ERROR reading from a socket");

	/* Compute sum of all the EM[p] signals */
	for(int i=0; i<3; i++) {
		int* emp = Encoder(codes[i], (ReadBuffers[i][2]-'0') );
		for(int j=0; j < 12; j++)
			em[j] += emp[j];
	}
	
	/*
	 Write off EM signal and Walsh Code that corresponds to the destination specified by each child process. (ROUTER BLOCK)
	 	Write buffer format =  [EM signal (0...11), Walsh Code of Sender (12...15)]
	 */
	for(int i=0; i < 3; i++) {
		for(int j=0; j < 12; j++)
			WriteBuffers[i][j] = em[j];
		if((ReadBuffers[i][1]-'0') == 1) // if dest. is 1, get code of sender
			for(int j= 12; j < 16; j++)
				WriteBuffers[0][j] = codes[((ReadBuffers[i][0]-'0')-1)][j-12];
		else if((ReadBuffers[i][1]-'0') == 2) // if dest. is 2, get code of sender
			for(int j= 12; j < 16; j++)
				WriteBuffers[1][j] = codes[((ReadBuffers[i][0]-'0')-1)][j-12];
		else if((ReadBuffers[i][1]-'0') == 3) // if dest. is 3, get code of sender
			for(int j= 12; j < 16; j++)
				WriteBuffers[2][j] = codes[((ReadBuffers[i][0]-'0')-1)][j-12];
	}

	int buff[16];
	//convert to endian friendly data and write back
	for(int i = 0; i < 3; i++) {
		for(int j=0; j < 16; j++)
			buff[j] = htonl(WriteBuffers[i][j]);
		n = write(newsockfd[i], buff, sizeof(int)*16);
		if (n < 0)
			error("ERROR writing to socket for child process 1");
		else
			sleep(1);
		close(newsockfd[i]);
	}
	
	return 0;
}
