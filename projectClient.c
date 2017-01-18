#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
int main(int argc, char** argv)
{

	char port[16];
	char ip[16];
	
	
	printf("Enter a port: ");
	fgets(port, 16, stdin);
	
	// Checking if port is valid
	if ( (atoi(port) > 65535) || (atoi(port) < 1) ){
		printf("Invalid port: out of range or not numeric\n");
		return 1;
	}
	
	printf("Enter an IP: ");
	fgets(ip, 16, stdin);
	
	// Checking that ip is divided into 4 sections with '.', and all are numeric.
	int count = 0;
	char *temp = malloc(16);
	char *tempIP = malloc(16);
	strcpy(tempIP, ip);
	temp = strtok(tempIP, ".");

	// temp will be Null after last token is read/found with strtok
	while(temp != NULL)
	{
		count++;
		int i = 0;
		
		// Loop checks each character of current section to see if numeric.
		while(i < strlen(temp) - 1)
		{
			if (isdigit(temp[i])){
				i++;
			}
			else{
				printf("Not a valid IP. Not Numeric.\n");
				return 1;
			}
		}
		// Check that value of section is less than 256.
		if( (atoi(temp)) > 255 || atoi(temp) < 0){
			printf("Not a valid IP. Out of range.\n");
			return 1;
		}
		temp = strtok(NULL, ".");
	}
	// If we didn't loop above 4 times, wrong number of sections.
	if (count != 4){
		printf("Not a valid IP. Wrong number of sections.\n");
		free(temp);
		free(tempIP);
		return 1;
	}
	else{
		free(tempIP);
		free(temp);
	}
	// Creating a socket
	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	/* Error Checking: Functions return < 0 if error, */

	if(sockfd < 0){
		printf("There was an error creating the socket\n");
		return 1;
	}
	// Setting up socket address
	struct sockaddr_in serveraddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(atoi(port));
	serveraddr.sin_addr.s_addr=inet_addr(ip);	

	int e = connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));

	/* Check for error */
	if(e < 0){
		printf("There was an error with connecting\n");
		return 1;
	}
	// Buffers used in the loop
	char *line = malloc(5000);
	char *sizeBuf = malloc(sizeof(char) * 10);
	char *buf = malloc(sizeof(char) * 512);
	char *validBuf = malloc(sizeof(char)* 1);
	
	// Infinite loop until exit request to allow multiple requests in one "session"
	while(1){
		
		printf("Enter the file path you request, or exit: ");
		fgets(line, 5000, stdin);
		if(strncmp(line, "exit", 4) == 0){
			printf("Leaving. Telling server goodbye\n");
			send(sockfd, line, strlen(line), 0);
			free(line);
			free(sizeBuf);
			free(buf);
			free(validBuf);
			close(sockfd);
			return 0;
		}
		//Eliminating new line from end of input - makes finding file easier on server
		line = strtok(line, "\n");
	
		// Sending file path over socket
		send(sockfd, line, strlen(line), 0);
		
		// Buffer for response if file found on server
		/* Check if file exists before creating anything */
		recv(sockfd, validBuf, 1, 0);
		if(strcmp(validBuf, "1") == 0){
			printf("File not found\n");
			continue;
		}
	
	
		// Getting size of file from server
		int size = 0;
		int test = recv(sockfd, sizeBuf, 10, 0);
		if(test < 0){
			printf("Error getting size\n");
			close(sockfd);
			return 1;
		}
		size = atoi(sizeBuf);
		// Buffer for file data. Comes in chunks of 512.
		FILE *file = fopen(line, "w+");
		
		//Need to recieve in loop, as server will send in packets (either implicitly or explicitly)
		int read;
		int bytes_left = size;
		while(bytes_left > 0)
		{
			read = recv(sockfd, buf, 512, 0);
			// Only write what was recieved, not extra bytes.
			fwrite(buf, sizeof(char), read, file);
			bytes_left = bytes_left -  read;
	
		}
		printf("Done copying file.\n");
	}
	return 0;
}
