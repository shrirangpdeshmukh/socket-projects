#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT 5000
#define MAX_MESSAGE_SIZE 64

/*
  Compile: gcc -o server 19CS01065_server.c -lpthread
  Run: ./server
  Please ensure that server is running before any client is executed.
*/

/* 
  Function to extract the components of the input buffer.
  Passes operation and both arguements via refernced variables.
*/
void analyseMessage(char *buffer, char *operation, float *arg1, float *arg2) {
  char * temp;
  char values[3][10];
  int i = 0;
  temp = strtok(buffer, " ");
  while (temp != NULL) {
    strcpy(values[i], temp);
    i++;
    if (i == 3) break;
    temp = strtok(NULL, " ");
  }

  strcpy(operation, values[0]);
  *arg1 = atof(values[1]);
  *arg2 = atof(values[2]);
}

/* 
  Function to perform the required operation on the arguements.
  Passes status flag and calculation result in refernced variables.
*/
void calculator(char *operation, float arg1, float arg2, float *ans, int *success) {
  if (strcmp(operation, "ADD") == 0) {
    *ans = arg1 + arg2;
    *success = 1;
  } else if (strcmp(operation, "SUB") == 0) {
    *ans = arg1 - arg2;
    *success = 1;
  } else if (strcmp(operation, "MUL") == 0) {
    *ans = arg1 * arg2;
    *success = 1;
  } else if (strcmp(operation, "DIV") == 0) {
    if (arg2 != 0) {
      *ans = arg1 / arg2;
      *success = 1;
    }
  }
}

/* 
  Function to perform calculation based on the input buffer.
  Returns the output string that is to be sent to the client.
*/
char *calculate(char *buffer) {
  float arg1 = 0.0, arg2 = 0.0, ans = 0.0;
  char operation[6];
  int success = 0;
  char*answer =(char*) malloc(sizeof(char) *MAX_MESSAGE_SIZE);

  analyseMessage(buffer, operation, &arg1, &arg2);

  // check if finish operation is called
  if (strcmp(operation, "FINISH")==0) {
      strcpy(answer, "FINISHED");
      return answer;
  }
  
  printf("Operation: %s, %f, %f\n", operation, arg1, arg2);

  calculator(operation, arg1, arg2, &ans, &success);
  
  if (success) {
    sprintf(answer, "%f", ans);
  } else {
    strcpy(answer,"Invalid request");
  }
  
  return answer; 
}

// Function to initialize socket file descriptor.
int initSocket() {
  int server_fd;
  struct sockaddr_in address;

  int opt = 1;
  int addrlen = sizeof(address);
  
  // create socket fd
  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  // set socket options and address options
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, & opt, sizeof(opt));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // binding and listening to the port.
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    printf("bind() failed");
    exit(1);
  } else {
    printf("bind() succeeded on PORT %d\n", PORT);
  }
  if (listen(server_fd, 1) == 0) {
    printf("listen() succeeded on PORT %d\n", PORT);
  } else {
    printf("listen() failed\n");
  }
  
  return server_fd;
}

// Thread Routine for incoming clients
void *calculatorClient(void *arg) {
  int *thread_id = (int *)arg;
  int instance_id = *thread_id;
  int socket_id = *thread_id;
  
  char *buffer = (char*) malloc(sizeof(char) * MAX_MESSAGE_SIZE);
  char *answer;
  
  // receive commands and send results back to client till client calls 'FINISH' 
  while(1) {
    memset(buffer, 0, sizeof(buffer));
    
    recv(socket_id, buffer, MAX_MESSAGE_SIZE, 0);
    
    answer = calculate(buffer);
    
    send(socket_id, answer, MAX_MESSAGE_SIZE, 0);
    
    if (strcmp(answer, "FINISHED")==0) break;

    free(answer);
  }
  printf("Closing connection for - %d\n", instance_id);
  close(socket_id);
  pthread_exit(NULL);
}

// main function
int main(int argc, char const *argv[]) {
  int server_fd = initSocket();
  int socket;
  
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  pthread_t thread_id;
  
  while(1){
    socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if(socket < 0){
      printf("accept() failed\n");
      continue;
    }
    printf("Server connected to client - %d\n", socket);
    pthread_create(&thread_id, NULL, calculatorClient, &socket);
  }

  return 0;
}