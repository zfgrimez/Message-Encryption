/*
Written by Zach Grimes
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>
#include <math.h>
void error(char *msg)
{
    perror(msg);
    exit(0);
}

void decode(int* msg, char pnum) {
	int dsig[12];
	int bbit[3]={0,0,0};
	int val=0;
	int j=2;
	//decode msg
	for(int i=0; i < 3; i++)
		for(int j=(i*4); j < ((i+1)*4);j++)
			dsig[j]=msg[j]*msg[(j+12)-(i*4)];

	for(int i=0; i < 3; i++) {
		for(int j = (i*4); j < ((i+1)*4); j++)
			bbit[i] += dsig[j];
		bbit[i]/=4;
	}
	for(int i=0; i<3;i++) {
		if(bbit[i]==1)
			val+=(int)(pow(2.0,(double)(j)));
		j--;
	}
	//Format output
	printf("\nChild %c\n",pnum);
	printf("Signal: ");
	for(int i=0; i < 12; i++)
		printf("%i ", msg[i]);
	printf("\nCode: ");
	for(int i=12; i < 16; i++)
		printf("%i ", msg[i]);
	printf("\nReceived value = %i\n", val);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
	pid_t pid;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char buffer[3], fileInputBuffer[9];
	int ReadBuffer[17];
	size_t current_size = 0;
	size_t size=5;
	
    if (argc < 3) {
	   fprintf(stderr,"Usage: %s hostname port < inputfile.txt\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
	
	/* <--- Read file input into a input file buffer ---> */
	char* inputline;
	inputline = (char *)malloc(size * sizeof(char));
	for(int i=0;i<3;i++){
		getline(&inputline,&size,stdin);
		fileInputBuffer[current_size]=((i+1)+'0');
		fileInputBuffer[(current_size+1)]=inputline[0];
		fileInputBuffer[(current_size+2)]=inputline[2];
		current_size+=3;
	}
	/* <--- Create and manage child processes ---> */
	for(int i=0;i<3;i++){
		if((pid=fork())==0) {
	/* <--- Begin process of establishing connection with encoder server ---> */
			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0)
				error("TCP CLIENT ERROR: opening socket failed (socket in use by another process)");
			server = gethostbyname(argv[1]);
			if (server == NULL) {
				fprintf(stderr,"TCP CLIENT ERROR: no such host found\n");
				exit(0);
			}
			bzero((char *) &serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			bcopy((char *)server->h_addr,
				  (char *)&serv_addr.sin_addr.s_addr,
				  server->h_length);
			serv_addr.sin_port = htons(portno);
			if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
				error("TCP CLIENT ERROR: failed to establish connection");
			
			bzero(buffer,3);
			if(i==0) { //load write buffer
				buffer[0]=fileInputBuffer[0];
				buffer[1]=fileInputBuffer[1];
				buffer[2]=fileInputBuffer[2];
			}
			else if(i==1) {
				buffer[0]=fileInputBuffer[3];
				buffer[1]=fileInputBuffer[4];
				buffer[2]=fileInputBuffer[5];
			}
			else {
				buffer[0]=fileInputBuffer[6];
				buffer[1]=fileInputBuffer[7];
				buffer[2]=fileInputBuffer[8];
			}
			printf("Child %c, sending value %c: to child process %c\n", buffer[0], buffer[2], buffer[1]);
			n = write(sockfd,buffer,strlen(buffer));
			/*
			 buffer format (char array) = [ch. process sending data(int 1-3), ch. process to receive information(int 1-3), code to send(int 0-7)]
			 */
			if (n < 0) {
				error("TCP CLIENT ERROR: writing to socket failed");
				_exit(1);
			}
			//prep, read, and translate buffer
			bzero(ReadBuffer, 17);
			n = read(sockfd, ReadBuffer, sizeof(int)*16);
			for(int i = 0; i < 16; i++)
				ReadBuffer[i] = ntohl(ReadBuffer[i]);
			if (n == -1) {
				error("TCP CLIENT ERROR: reading from socket failed");
				_exit(1);
			}
			else {
				decode(ReadBuffer, buffer[0]);
				_exit(1);
			}
		}
		sleep(1);
	}
	//call parent to loop three times
	for (int i=0;i<3;i++)
		wait(NULL);
	
    return 0;
}

