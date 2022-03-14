#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define PORT 5000
#define MAX_MESSAGE_SIZE 1024

/*
  Compile: gcc -o server 19CS01065_server.c
  Run: ./server
  Please ensure that server is running before client is executed.
  Client always sends the first message.
*/

char *getIP() {
    char hostbuffer[256];
	char *IPbuffer;
	struct hostent *host_entry;

	gethostname(hostbuffer, sizeof(hostbuffer));
	host_entry = gethostbyname(hostbuffer);
	IPbuffer = inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
    
    return IPbuffer;
}

int isFinished(int socket, char *message) {
    if (strcmp(message, "good bye") == 0 ) {
		printf("The chat is being disconnected.\n");
		shutdown(socket, 2);
        return 1;
    }
    return 0;
}

void sendMessage(int socket, char *message) {
    printf("Enter you Message: ");
    scanf("%[^\n]%*c", message);
				
    send(socket , message , strlen(message) , 0);
}

void receiveMessage(int socket, char *buffer) {
    read( socket , buffer, MAX_MESSAGE_SIZE);
	printf("Received Message: %s\n",buffer );
}

int main(int argc, char const *argv[])
{
	int server, server_socket;
	struct sockaddr_in address;
    
	int opt = 1;
	int addrlen = sizeof(address);
    
	char buffer[MAX_MESSAGE_SIZE] = {0};
	char message[MAX_MESSAGE_SIZE];
    
    char *IPbuffer = getIP();
	
	server = socket(AF_INET, SOCK_STREAM, 0);
	
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons( PORT );
	
	bind(server, (struct sockaddr *)&address, sizeof(address));
	listen(server, 1);
    
	server_socket = accept(server, (struct sockaddr *)&address, (socklen_t*)&addrlen);

	printf("Server connected to client\n");
		
	read(server_socket , buffer, MAX_MESSAGE_SIZE);
	printf("Hello from the other side!\nYou are connected to: %s\n",buffer);
	
    send(server_socket , IPbuffer , strlen(IPbuffer) , 0 );
   

	while(1) {

        memset(buffer, 0, sizeof(buffer));
        
        receiveMessage(server_socket, buffer);
		
		if (isFinished(server_socket, buffer)) break;
        
        sendMessage(server_socket, message);
		
		if (isFinished(server_socket, message)) break;
		
	}
}
