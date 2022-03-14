#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT 5000
#define MAX_MESSAGE_SIZE 1024
#define MAX_CLIENTS 30
#define REFRESH_DETAILS_COMMAND "RS"
#define ESTABLISH_CHAT_COMMAND "CHAT"
#define CONNECTION_CLOSE_COMMAND "CLOSE"
#define GOODBYE_MESSAGE "GOODBYE!"

typedef struct client
{
	int socket_id;
	int status;
	int partner;
} client;

int totalClients = 0;
client *clientDetails[MAX_CLIENTS];
int messageSize = sizeof(char) *MAX_MESSAGE_SIZE;

sem_t mutex;

int initSocket();
void *chatClient(void *arg);
client* createClient(int socket_id);
void destroyClient(client *prevClient);
char *connectedClientsDetails();
void sendConnectedClientMessage(int socket_id);
void sendWelcomeMessage(int socket_id);
int parseMessage(int socket_id, char *buffer, int *arg);
int findIndex(int socket_id);
void establishChat(int sender_id, int receiver_id, int sender_index);
void destroyChat(int sender_id, int receiver_id, int sender_index);


// main function
int main(int argc, char const *argv[])
{
	system("clear");
	printf("Initializing chat app server\n");

	sem_init(&mutex, 0, 1);

	int server_fd = initSocket();
	int socket;

	struct sockaddr_in address;
	int addrlen = sizeof(address);
	pthread_t thread_id;

	while (1)
	{
		socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t*) &addrlen);

		if (socket < 0)
		{
			printf("accept() failed\n");
			continue;
		}

		printf("Server connected to client - %d\n", socket);
		pthread_create(&thread_id, NULL, chatClient, &socket);
	}

	sem_destroy(&mutex);
	return 0;
}

// Function to initialize socket file descriptor.
int initSocket()
{
	int server_fd;
	struct sockaddr_in address;

	int opt = 1;
	int addrlen = sizeof(address);

	// create socket fd
	server_fd = socket(AF_INET, SOCK_STREAM, 0);

	// set socket options and address options
	setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	// binding and listening to the port.
	if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0)
	{
		printf("bind() failed");
		exit(1);
	}
	else
	{
		printf("bind() succeeded on PORT %d\n", PORT);
	}

	if (listen(server_fd, 1) == 0)
	{
		printf("listen() succeeded on PORT %d\n", PORT);
	}
	else
	{
		printf("listen() failed\n");
	}

	return server_fd;
}

// Thread Routine for incoming clients
void *chatClient(void *arg)
{
	int *thread_id = (int*) arg;
	int socket_id = *thread_id;
	int index;

	sem_wait(&mutex);
	index = totalClients++;
	sem_post(&mutex);

	client *newClient = createClient(socket_id);
	clientDetails[index] = newClient;

	char *answer = (char*) malloc(messageSize), *buffer = (char*) malloc(messageSize);
	sprintf(answer,"%d", socket_id);
	send(socket_id, answer, messageSize, 0);
	sendWelcomeMessage(socket_id);

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		recv(socket_id, buffer, messageSize, 0);

		printf("Client %d: %s\n", socket_id, buffer);

		// if client is not connected to chat then analyse its messages, else just send them to required receiver
		//RS - 1, CHAT -2, CLOSE - 3, Invalid = -1

		int arg = -1;
		if (clientDetails[index]->status == 1)
		{
			int res = parseMessage(socket_id, buffer, &arg);
			if (res == 1)
			{
				sendConnectedClientMessage(socket_id);
			}
			else if (res == 2)
			{
				establishChat(socket_id, arg, index);
			}
			else if (res == 3)
			{
				break;
			}
			else
			{
				strcpy(answer, "Invalid command!\n");
				send(socket_id, answer, messageSize, 0);
			}
		}
		else
		{
			// forawrd to the receiver
			int partner = clientDetails[index]->partner;
			printf("Message\t%d -> %d: %s\n", socket_id, partner, buffer);
			send(partner, buffer, messageSize, 0);

			if (strcmp(buffer, GOODBYE_MESSAGE) == 0)
			{
				destroyChat(socket_id, partner, index);
			}
		}
	}

	printf("Closing connection for - %d\n", socket_id);
	strcpy(answer, "CLOSED");
	send(socket_id, answer, messageSize, 0);
	destroyClient(clientDetails[index]);
	close(socket_id);
	free(answer);
	free(buffer);
	pthread_exit(NULL);
}

// Function get a string with details of all the clients connected to the server.
char *connectedClientsDetails()
{
	char *message = (char*) malloc(messageSize), *details = (char*) malloc(messageSize);
	int i, currentClients = 0;
	sem_wait(&mutex);
	for (i = 0; i < totalClients; i++)
	{
		if (clientDetails[i]->socket_id > 0)
		{
			currentClients++;
			char *temp = (char*) malloc(messageSize);
			sprintf(temp, "Instance ID: %d\tStatus: %s\n", clientDetails[i]->socket_id, clientDetails[i]->status == 1 ? "FREE" : "BUSY");
			strcat(message, temp);
			free(temp);
		}
	}

	sem_post(&mutex);
	sprintf(details, "Total connected clients: %d\n", currentClients);
	strcat(details, message);
	free(message);
	return details;
}

// Function to send welcome message to a client
void sendWelcomeMessage(int socket_id)
{
	char *welcomeMessage = (char*) malloc(messageSize);
	char *details = connectedClientsDetails();
	sprintf(welcomeMessage, "Welcome to the Chat App!\nYour Instance ID: %d\n\n%s\n", socket_id, details);
	send(socket_id, welcomeMessage, messageSize, 0);
	free(welcomeMessage);
	free(details);
}

// Function to send details of all connected clients to a client.
void sendConnectedClientMessage(int socket_id)
{
	char *clientInfoMsg = (char*) malloc(messageSize);
	char *details = connectedClientsDetails();
	sprintf(clientInfoMsg, "\n\n--------------- Client Information ---------------\nYour Instance ID: %d\n\n%s", socket_id, details);
	send(socket_id, clientInfoMsg, messageSize, 0);
	free(clientInfoMsg);
	free(details);
}

// Function to parse message from client for commands.
int parseMessage(int socket_id, char *buffer, int *arg)
{
	char *temp, values[2][10];
	int i = 0;
	temp = strtok(buffer, " ");
	while (temp != NULL)
	{
		strcpy(values[i], temp);
		i++;
		if (i == 2) break;
		temp = strtok(NULL, " ");
	}

	if (strcmp(values[0], REFRESH_DETAILS_COMMAND) == 0) return 1;
	if (strcmp(values[0], ESTABLISH_CHAT_COMMAND) == 0)
	{
		*arg = atoi(values[1]);
		return 2;
	}
	if (strcmp(values[0], CONNECTION_CLOSE_COMMAND) == 0) return 3;
	return -1;
}

// Function to establish Chat connection
void establishChat(int sender_id, int receiver_id, int sender_index)
{
	char *response = (char*) malloc(messageSize);
	int isValid = 1;

	if (receiver_id == sender_id)
	{
		sprintf(response, "You can't chat with yourself!\n");
		isValid = 0;
	}

	int receiver_index = findIndex(receiver_id);

	if (receiver_index == -1)
	{
		sprintf(response, "Requested Client ID %d is invalid.\n", receiver_id);
		isValid = 0;
	}
	else if (clientDetails[receiver_index]->status != 1)
	{
		sprintf(response, "Requested Client ID %d is not currently available to chat.\n", receiver_id);
		isValid = 0;
	}

	if (!isValid)
	{
		send(sender_id, response, messageSize, 0);
		free(response);
		return;
	}

	clientDetails[receiver_index]->status = 0;
	clientDetails[sender_index]->status = 0;
	clientDetails[receiver_index]->partner = sender_id;
	clientDetails[sender_index]->partner = receiver_id;

	sprintf(response, "C_SUCCESS %d", receiver_id);
	send(sender_id, response, messageSize, 0);

	sprintf(response, "C_SUCCESS %d", sender_id);
	send(receiver_id, response, messageSize, 0);

	free(response);
}

// Function to destroy Chat connection
void destroyChat(int sender_id, int receiver_id, int sender_index)
{
	char *response = (char*) malloc(messageSize);

	int receiver_index = findIndex(receiver_id);

	if (receiver_index == -1)
	{
		strcpy(response, "Something went wrong in finidng the index\n");
		send(sender_id, response, messageSize, 0);
		free(response);
		return;
	}

	clientDetails[receiver_index]->status = 1;
	clientDetails[sender_index]->status = 1;
	clientDetails[receiver_index]->partner = -1;
	clientDetails[sender_index]->partner = -1;

	strcpy(response, "C_E_SUCCESS");
	send(sender_id, response, messageSize, 0);
	send(receiver_id, response, messageSize, 0);

	free(response);
}

// Utility Functions
client* createClient(int socket_id)
{
	client *newClient = (client*) malloc(sizeof(client));
	newClient->socket_id = socket_id;
	newClient->status = 1;
	newClient->partner = -1;
	return newClient;
}

void destroyClient(client *prevClient)
{
	prevClient->socket_id = -1;
	prevClient->status = -1;
	prevClient->partner = -1;
}

int findIndex(int socket_id)
{
	int index = -1, i;
	sem_wait(&mutex);
	for (i = 0; i < totalClients; i++)
	{
		if (clientDetails[i]->socket_id == socket_id)
		{
			index = i;
			break;
		}
	}

	sem_post(&mutex);
	return index;
}