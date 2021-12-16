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

#define MAX_MESSAGE_LEN 256
#define MAX_NAME_LEN 128

#define NEW_USER_SIGNAL 0
#define QUIT_USER_SIGNAL -1
#define ONLINE_USER_SIGNAl 1

typedef struct Message
{
	int user_id;
	char user_name[MAX_NAME_LEN];
	char str[MAX_MESSAGE_LEN];
} Message;

void *sendThreadClient();

int sock_main;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

char username[MAX_NAME_LEN];

int main()
{
	int th_id;
	Message buff;
	pthread_t th_send;
	struct sockaddr_in addr;

	printf("Name: ");
	scanf("%s", username);
	getchar();

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Socket open failed\n");

		exit(1);
	}

	if (connect(sock_main, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		printf("Connect failed\n");

		exit(4);
	}

	Message initMsg;

	initMsg.user_id = NEW_USER_SIGNAL;
	strcpy(initMsg.user_name, username);
	initMsg.str[0] = '\0';

	send(sock_main, (char*)&initMsg, sizeof(Message), 0);

	th_id = pthread_create(&th_send, NULL, sendThreadClient, 0);

	if (th_id < 0)
	{
		printf("send thread creation failed\n");

		exit(1);
	}

	while (1)
	{
		memset(&buff, 0, sizeof(buff));

		if (recv(sock_main, (char*)&buff, sizeof(buff), 0) > 0)
		{
			printf("User %s: %s\n", buff.user_name, buff.str);
		}
		else
		{
			printf("Disconnected\n");

			exit(5);
		}
	}

	return 0;
}

void *sendThreadClient()
{
	Message tmp;
	int count;
	char quit[6] = "/quit";

	while (1)
	{
		memset(&tmp, 0, sizeof(tmp));

		fgets(tmp.str, MAX_MESSAGE_LEN, stdin);

		strcpy(tmp.user_name, username);
		tmp.str[strlen(tmp.str) -1] = '\0';

		if (strcmp(tmp.str, quit) == 0)
		{
			send(sock_main, (char*)&tmp, sizeof(Message), 0);

			printf("quit\n");

			exit(1);
		}

		tmp.user_id = -1;

		count = send(sock_main, (char*)&tmp, sizeof(Message), 0);
	}
}


