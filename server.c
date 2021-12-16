#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define PORT 30325

#define MAX_BUFF 100
#define MAX_MESSAGE_LEN 256
#define MAX_NAME_LEN 128
#define MAX_USER_COUNT 10

#define NEW_USER_SIGNAL 0
#define QUIT_USER_SIGNAL -1
#define ONLINE_USER_SIGNAL 1

typedef struct Message 
{
	int user_id;
	char user_name[MAX_NAME_LEN];
	char str[MAX_MESSAGE_LEN];
} Message;

typedef struct User
{
	int id;
	int status;
	char name[MAX_NAME_LEN];
} User;

void *sendThread();
void *recvThread(void *data);

int isFull();
int isEmpty();
int enqueue(Message item);
Message* dequeue();

int sock_main, sock_client[MAX_USER_COUNT];
Message* msgbuff;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

User users[MAX_USER_COUNT];

int front = -1;
int rear = -1;

int count = 0;

int main()
{
	int th_id;
	Message buff;
	pthread_t th_send;

	struct sockaddr_in addr;

	pthread_t th_recv[MAX_USER_COUNT];

	msgbuff = (Message *)malloc(sizeof(Message) * MAX_BUFF);

	th_id = pthread_create(&th_send, NULL, sendThread, 0);

	if (th_id < 0)
	{
		printf("Send thread creation failed\n");

		exit(1);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Socket open failed\n");

		exit(1);
	}

	if (bind(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		printf("Bind failed\n");

		exit(2);
	}

	if (listen(sock_main, 5) == -1)
	{
		printf("Listen failed\n");

		exit(3);
	}

	while (1)
	{
		if ((sock_client[count] = accept(sock_main, NULL, NULL)) == -1)
		{
			printf("Accept failed\n");

			continue;
		}	
		else
		{
			if (count < MAX_USER_COUNT)
			{
				int idx = count;

				th_id = pthread_create(&th_recv[count], NULL, recvThread, (void *)&idx);

				if (th_id < 0)
				{
					printf("Receive thread %d create failed\n", count + 1);
				
					exit(1);
				}
				
				count++;
			}
		}
	}

	return 0;
}

void *sendThread()
{
	Message *tmp;

	printf("Send thread start\n");

	while(1)
	{
		if ((tmp = dequeue()) != NULL)
		{
			for (int i = 0; i < MAX_USER_COUNT; i++)
			{
				if (i != tmp->user_id && users[i].status != QUIT_USER_SIGNAL)
				{
					send(sock_client[i], (char*)tmp, sizeof(Message), 0);
				}
			}
			
			usleep(1000);
		}
	}
}

void* recvThread(void* data)
{
	Message buff;
	int thread_id = *((int*)data);
	char quit[6] = "/quit";

	memset(&buff, 0, sizeof(Message));

	while(recv(sock_client[thread_id], (char*)&buff, sizeof(buff), 0))
	{
		if (strcmp(buff.str, quit) == 0)
		{
			users[thread_id].status = QUIT_USER_SIGNAL;

			printf("User %s quited (Thread#%d)\n\n", users[thread_id].name, thread_id);

			printf("=============== Users status ===============\n\n");

			for (int i = 0; i < count; i++)
			{
				printf("Users: %s, ID: %d, thread_id: %d, status: %d\n",
						users[i].name, users[i].id, i, users[i].status);
			}

			printf("============================================\n\n");

			continue;
		}

		if (buff.user_id == NEW_USER_SIGNAL)
		{
			users[thread_id].id = thread_id;
			strcpy(users[thread_id].name, buff.user_name);
			users[thread_id].status = ONLINE_USER_SIGNAL;

			printf("Users %s joined (Thread#%d)\n\n", users[thread_id].name, users[thread_id].id);

			printf("=============== Users status ===============\n\n");

			for (int i = 0; i < count; i++)
			{
				printf("Users: %s, Id: %d, thread_id: %d, status: %d\n",
						users[i].name, users[i].id, i, users[i].status);
			}

			printf("============================================\n\n");
		}
		else
		{
			buff.user_id = thread_id;

			if (enqueue(buff) == -1)
			{
				printf("Message buffer full\n");
			}
		}
	}
}

int isFull()
{
	if ((front == rear + 1) || (front == 0 && rear == MAX_BUFF -1)) 
	{
		return 1;
	}

	return 0;
}

int isEmpty()
{
	if (front == -1)
	{
		return 1;
	}

	return 0;
}

int enqueue(Message item)
{
	if (isFull())
	{
		return -1;
	}
	else
	{
		pthread_mutex_lock(&mutex);

		if (front == -1)
		{
			front = 0;
		}

		rear = (rear + 1) % MAX_BUFF;

		msgbuff[rear].user_id = item.user_id;
		strcpy(msgbuff[rear].user_name, item.user_name);
		strcpy(msgbuff[rear].str, item.str);

		pthread_mutex_unlock(&mutex);
	}

	return 0;
}

Message* dequeue()
{
	Message *item;

	if (isEmpty())
	{
		return NULL;
	}
	else
	{
		pthread_mutex_lock(&mutex);

		item = &msgbuff[front];

		if (front == rear)
		{
			front = -1;
			rear = -1;
		}
		else 
		{
			front = (front + 1) % MAX_BUFF;
		}

		pthread_mutex_unlock(&mutex);

		return item;
	}
}
