#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 5000
#define MAX_MESSAGE_SIZE 1024

/*
  Compile: gcc -o client 19CS01065_client.c
  Run: ./client
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
				
    send(socket , message , strlen(message) , 0 );
}

void receiveMessage(int socket, char *buffer) {
    read( socket , buffer, MAX_MESSAGE_SIZE);
	printf("Received Message: %s\n",buffer );
}

int main(int argc, char const *argv[])
{
	int client = 0;
	struct sockaddr_in server_address;
    
	char message[MAX_MESSAGE_SIZE];
	char buffer[MAX_MESSAGE_SIZE] = {0};
	
	char *serverIPAddress = "127.0.0.1";
	
	char *IPbuffer = getIP();
	
    
	client = socket(AF_INET, SOCK_STREAM, 0);
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);

	inet_pton(AF_INET, serverIPAddress, &server_address.sin_addr);
    
	if (connect(client, (struct sockaddr *)&server_address, sizeof(server_address)) ==0 ) {
		
		printf("Client Conected to Server\n");

        send(client , IPbuffer, strlen(IPbuffer) , 0 );
		
		read(client , buffer, MAX_MESSAGE_SIZE);
	    printf("Hello from the other side!\nYou are connected to: %s\n",buffer);
	
	}
    
    while (1) {	
        
        memset(buffer, 0, sizeof(buffer));
		        
        sendMessage(client, message);
		
		if (isFinished(client, message)) break;
	
        receiveMessage(client, buffer);
		
		if (isFinished(client, buffer)) break;
	
	}
    
	return 0;
}
