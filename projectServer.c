#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>

// Making files global for easier thread use
char *files[5000];

int writeThread(int socket)
{
	if(files[socket] == NULL)
		return -1;	
	/* Reading the file and sending in chunks 
 	* Starting by telling the client if file was found */
	FILE *file = fopen(files[socket], "r");
	if(file == NULL){
		printf("File was not found. Telling client.\n");
		send(socket, "1", 1, 0);
		return -1;
		}
	else{
		send(socket, "0", 1, 0);
	}

	// Sending Client size of file;
	struct stat this_stat;
	stat(files[socket], &this_stat);
	int size = this_stat.st_size;
	char *sizeBuf = malloc(sizeof(char) * 10);
	sprintf(sizeBuf, "%d", size);
	send(socket, sizeBuf, 10, 0);
	free(sizeBuf);
	// Read in chunks, send that chunk
	char *toSend = malloc(512);
	int temp = 1;
	int bytes_left = size;
	while( bytes_left > 0)
	{	
	// Sending 512 bytes each time.
	temp = fread(toSend, sizeof(char), 512, file);
	bytes_left -= temp;
	send(socket, toSend, temp, 0);
	}
	free(toSend);
	return 0;
}


int main(int argc, char** argv)
{
	
	int i;
	for(i = 0; i < 5000; i++)
		files[i] = "";

	char port[16];
	printf("Enter port: ");
	
	fgets(port, 16, stdin);

	if ((atoi(port) <= 0) || (atoi (port) > 65535))
	{
		printf("Invalid port number.\n");
		return 1;
	}

	int sockfd = socket(AF_INET,SOCK_STREAM,0);

	/* Error Checking: Functions return < 0 if error, no exceptions really */

	if(sockfd < 0){
		printf("There was an error creating the socket\n");
		return 1;
	}

	fd_set sockets;
	FD_ZERO(&sockets);	

	/* Server end: Server struct is mostly same, but different for IP */
	struct sockaddr_in serveraddr, clientaddr;
	serveraddr.sin_family=AF_INET;
	serveraddr.sin_port=htons(atoi(port));
	serveraddr.sin_addr.s_addr=INADDR_ANY;



	int e = bind(sockfd, (struct sockaddr*)&serveraddr,sizeof(serveraddr));
	if (e < 0)
	{
		printf("Error binding\n");
		return 1;
	}
	listen(sockfd, 10);
	
	// Adding sockfd to set of sockets
	FD_SET(sockfd, &sockets);
	int socketsSize = 1;
	/* recv is used to recieve data. The socket, the variable to save to, the max data to recieve, options */
	/* to send reply back to client, just used send over clientsocket */
	while(1)
	{
		socklen_t len = sizeof(clientaddr);
		
		fd_set tmp_set = sockets;

		select(FD_SETSIZE, &tmp_set, NULL, NULL, NULL);

		int i;
		
		for(i = 0; i < FD_SETSIZE; ++i)
		{
			if(FD_ISSET(i, &tmp_set)){
			if(i == sockfd)
			{
				int clientSocket = accept(sockfd, (struct sockaddr*)&clientaddr, &len);
				if(clientSocket > 0)
				{
					printf("Client connected and accepted\n");
					FD_SET(clientSocket, &sockets);
					socketsSize++;
				}
				else{
					printf("Error accepting/connecting \n");
					return 1;
				}
			}
			else {
				char line[5000];
				int n = recv(i, line, 5000, 0);
				line[n] = '\0';
				if(strncmp(line, "exit", 4) == 0){
					printf("Client is closing\n");
					close(i);
					FD_CLR(i, &sockets);
				}
				else{
					printf("Client requested file: %s\n", line);	
					files[i] = line;	
				



					pthread_t threads[FD_SETSIZE];


					pthread_create(&threads[i], NULL, (void *)writeThread, (void *) (intptr_t) i);
			
				}
			}
			}
		}

	}
	return 0;
}
