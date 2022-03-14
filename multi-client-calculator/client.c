#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#define PORT 5000
#define MAX_MESSAGE_SIZE 64

/*
  Compile: gcc -o client 19CS01065_client.c
  Run: ./client
  Please ensure that server is running before any client is executed.
  Run multiple client programs in different terminals to observe the server serving the clients simultaneously.
  Instructions supported: 
    ADD x y, SUB x y, MUL x y, DIV x y, FINISH => to the terminate the connection.
*/

int main(int argc, char  const * argv[]) {
  //create client fd
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in server_address;
  char *serverIPAddress = "127.0.0.1";

  if (client_fd < 0) {
    printf("Socket creation failed.\n");
    exit(1);
  } else {
    printf("Client Socket created successfully.\n");
  }

  // set server_address options
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = INADDR_ANY;
  server_address.sin_port = htons(PORT);
  inet_pton(AF_INET, serverIPAddress, &server_address.sin_addr);

  // connect to the server
  int connection = connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address));

  if (connection < 0) {
    printf("connect() failed\n");
    exit(1);
  }

  printf("Client conected to server successfully\n");

  char *buffer = (char*) malloc(sizeof(char) * MAX_MESSAGE_SIZE);
  char *message = (char*) malloc(sizeof(char) * MAX_MESSAGE_SIZE);
  
  // send commands to the server and receive responses, till the server sends FINISHED response.
  while(1) {
    printf("Enter Operation: ");
    scanf("%[^\n]%*c", message);
    
    send(client_fd, message, strlen(message), 0);
    recv(client_fd, buffer, MAX_MESSAGE_SIZE, 0);
    
    if(strcmp(buffer,"FINISHED") == 0) break;
    printf("%s\n", buffer);
  }
  printf("Closing connection.\n");
  close(client_fd);
  return 0;
}