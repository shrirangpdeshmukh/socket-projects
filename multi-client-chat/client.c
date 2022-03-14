#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT 5000
#define MAX_MESSAGE_SIZE 1024
#define GOODBYE_MESSAGE "GOODBYE!"

typedef struct msg
{
	char *msgLine;
	struct msg * next;
} msg;

typedef struct list
{
	struct msg *front, *rear;
	int count;
} list;

int initClient();
void *send_thread(void *arg);
void *recv_thread(void *arg);
msg* newMsg(char *message, int sender);
list* createList();
void printMessages(list *l);
void addMsg(list *msgList, char *message, int sender);
void refreshChat();

int isOpen = 0, isChatting = 0, partner = -1, server_client_fd;

sem_t mutex, accessList;

int messageSize = sizeof(char) *MAX_MESSAGE_SIZE;

list * msgs;

int main(int argc, char const *argv[])
{
	system("clear");
	printf("Initializing chat app client\n");

	//create client fd
	int client_fd = initClient();

	sem_init(&mutex, 0, 1);
	sem_init(&accessList, 0, 1);

	char *buffer = (char*) malloc(messageSize);
	recv(client_fd, buffer, messageSize,0);
	server_client_fd = atoi(buffer);
	memset(buffer, 0, sizeof(buffer));
	recv(client_fd, buffer, messageSize, 0);
	printf("\n%s\n", buffer);
	free(buffer);

	pthread_t sender, receiver;

	pthread_create(&sender, NULL, send_thread, &client_fd);
	pthread_create(&receiver, NULL, recv_thread, &client_fd);

	while (1)
	{
		int shouldBreak = 0;
		sem_wait(&mutex);
		shouldBreak = !isOpen;
		sem_post(&mutex);
		if (shouldBreak)
		{
			pthread_cancel(sender);
			pthread_cancel(receiver);
			break;
		}
	}

	printf("Closed connection with server.\n");
	sem_destroy(&mutex);
	sem_destroy(&accessList);
	close(client_fd);
	return 0;
}

int initClient()
{
	int client_fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	char *serverIPAddress = "127.0.0.1";

	if (client_fd < 0)
	{
		printf("Socket creation failed.\n");
		exit(1);
	}
	else
	{
		printf("Client Socket created successfully.\n");
	}

	// set server_address options
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(PORT);
	inet_pton(AF_INET, serverIPAddress, &server_address.sin_addr);

	// connect to the server
	int connection = connect(client_fd, (struct sockaddr *) &server_address, sizeof(server_address));

	if (connection < 0)
	{
		printf("connect() failed\n");
		exit(1);
	}

	isOpen = 1;

	printf("Client conected to server successfully\n");

	return client_fd;
}

// thread routnie for sending messages to the server
void *send_thread(void *arg)
{
	int *fd = (int*) arg;
	int client_fd = *fd;

	printf("Enter command to interact with the server.\nSupported Commands:\n1.RS - To get the list of available clients\n2.CHAT 'id' - Chat with the client of given id\n3.CLOSE - Close the connection\n\n");

	while (1)
	{
		// sleep(1);
		char *message = (char*) malloc(messageSize), *reply = (char*) malloc(messageSize);

		if (isChatting)
		{
			scanf("%[^\n]%*c", reply);
			printf("\nSent Message: %s\n", reply);
			sem_wait(&accessList);
			addMsg(msgs, reply, server_client_fd);
			refreshChat();
			sem_post(&accessList);
			if (strlen(reply)-1 > 0)
			{
				send(client_fd, reply, messageSize, 0);
			}
			
			if (strcmp(reply, GOODBYE_MESSAGE) == 0)
			{
				printf("Ending the conversation with %d\n", partner);
				isChatting = 0;
				partner = -1;
				free(msgs);
			}
		}
		else
		{
			printf("Enter command: ");
			scanf("%[^\n]%*c", reply);
			if (strlen(reply)-1 > 0)
			{
				send(client_fd, reply, strlen(reply), 0);
			}
		}

		memset(reply, 0, sizeof(reply));
		fflush(stdin);
	}
}

// thread for receving the messages from server
void *recv_thread(void *arg)
{
	int *fd = (int*) arg;
	int client_fd = *fd;
	char *buffer = (char*) malloc(messageSize);

	while (1)
	{
		memset(buffer, 0, sizeof(buffer));
		recv(client_fd, buffer, messageSize, 0);
		printf("\nReceived Message: %s\n", buffer);
		if (isChatting)
		{
			sem_wait(&accessList);
			addMsg(msgs, buffer, partner);
			refreshChat();
			sem_post(&accessList);

			if (strcmp(buffer, GOODBYE_MESSAGE) == 0)
			{
				printf("Ending the conversation with %d\n", partner);
				isChatting = 0;
				partner = -1;
				free(msgs);
			}
		}
		else
		{
			char *temp, *t2 = (char*) malloc(messageSize);
			strcpy(t2, buffer);
			temp = strtok(t2, " ");

			if (strcmp(temp, "CLOSED") == 0)
			{
				sem_wait(&mutex);
				isOpen = 0;
				sem_post(&mutex);
				free(buffer);
				break;
			}
			else if (strcmp(temp, "C_SUCCESS") == 0)
			{
				char *temp2, values[2][10];
				int i = 0;
				temp2 = strtok(buffer, " ");
				while (temp2 != NULL)
				{
					strcpy(values[i], temp2);
					i++;
					if (i == 2) break;
					temp2 = strtok(NULL, " ");
				}

				partner = atoi(values[1]);
				isChatting = 1;
				msgs = createList();
				system("clear");
				printf("Starting conversation with %d\n", partner);

			}

			free(t2);
		}

	}
}

// Utility functions for supporting list of messages.
msg* newMsg(char *message, int sender)
{
	msg *temp = (msg*) malloc(sizeof(msg));
	temp->next = NULL;
	temp->msgLine = (char*) malloc(messageSize);
	sprintf(temp->msgLine, "%d: %s", sender, message);
	return temp;
}

list* createList()
{
	list *l = (list*) malloc(sizeof(list));
	l->front = l->rear = NULL;
	l->count = 0;
	return l;
}

void printMessages(list *l)
{
	msg *temp = l->front;
	while (temp != NULL)
	{
		printf("%s\n", temp->msgLine);
		temp = temp->next;
	}
}

void addMsg(list *msgList, char *message, int sender)
{
	int shouldAdd = 1;
	

	if (!shouldAdd)
	{
		return;
	}

	msg *incomingMsg = newMsg(message, sender);
	if (msgList->rear == NULL)
	{
		msgList->front = msgList->rear = incomingMsg;
		return;
	}

	msgList->rear->next = incomingMsg;
	msgList->rear = incomingMsg;
	msgList->count += 1;
}

void refreshChat()
{
	system("clear");
	printf("------ Chat Record ------\n");
	printMessages(msgs);
	printf("--------------------------\n");
}