// 1. HEADERS
// from main.c
#include <string.h>		//strlen
#include <unistd.h>		//write, close
#include <sys/socket.h>	//socket, listen, bind, accept, send, recv
#include <netinet/in.h> // AF_INET
// from me
#include <strings.h>	//bzero
#include <stdlib.h>		//atoi, exit
#include <poll.h>		//poll
#include <stdio.h>		//sprintf



// 2. MACROS, STRUCT, GLOBAL VARS
//MACROS
enum
{
	RECV_BUFFER_SIZE = 100000,
	SEND_BUFFER_SIZE = 100050,
	MAX_CLIENT = 1024
};

//STRUCT
typedef struct s_client
{
	int id;
	char msg[RECV_BUFFER_SIZE];
}	t_client;

//GLOBAL VARS
struct pollfd pollfds[MAX_CLIENT + 1];
t_client client[MAX_CLIENT + 1];
int fdCount = 0;
int currId = 0;
char sendBuffer[SEND_BUFFER_SIZE];
char recvBuffer[RECV_BUFFER_SIZE];



// 3. LIST ALL HELPER FN - GOT 6
void fatalError_exit();
void newConnection(int);
void broadcastMsg(int);
void handleClient(int, int);
void processMsg(int, int, int);
void leftGpChat(int, int);



//4. INT MAIN
int main(int ac, char **av)
{
	if (ac != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(2);
	}

	int sockfd;//, connfd, len;
	struct sockaddr_in servaddr;//, cli; 

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
		fatalError_exit();
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 

	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
		fatalError_exit();
	if (listen(sockfd, 10) != 0) 
		fatalError_exit();
	
	// set listener into pollfds
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;
	fdCount++;

	// loop!
	while (1)
	{
		if (poll(pollfds, fdCount, -1) == -1)
			fatalError_exit();
		if (pollfds[0].revents & POLLIN)
			newConnection(sockfd);
		for (int i = 1; i < fdCount; ++i)
			if (pollfds[i].revents & POLLIN)
				handleClient(pollfds[i].fd, i);
	}
	return 0;
}

void fatalError_exit()
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

void newConnection(int sockfd)
{
	int connfd;
	socklen_t len;
	struct sockaddr_in cli; 

	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0) 
		fatalError_exit();
	if (fdCount > MAX_CLIENT)
	{
		close(connfd);
		return;
	}

	// add pollfds and fdcount
	int currFdCount = fdCount++;
	pollfds[currFdCount].fd = connfd;
	pollfds[currFdCount].events = POLLIN;

	// add currId & client
	client[currFdCount].id = currId++;

	// tell other clients who enter into the gp chat
	bzero(sendBuffer, SEND_BUFFER_SIZE);
	sprintf(sendBuffer, "server: client %d just arrived\n", client[currFdCount].id);
	broadcastMsg(connfd);
}

void broadcastMsg(int clientFd)
{
	for (int i = 1; i < fdCount; ++i)
		if (pollfds[i].fd != clientFd && pollfds[i].fd != -1)
			send(pollfds[i].fd, sendBuffer, strlen(sendBuffer), 0);
}

void handleClient(int clientFd, int pollIdx)
{
	// do recv
	ssize_t recvRet = recv(clientFd, recvBuffer, RECV_BUFFER_SIZE, 0);
	if (recvRet <= 0)
		leftGpChat(pollIdx, clientFd);
	else
		processMsg(recvRet, pollIdx, clientFd);
}

void leftGpChat(int pollIdx, int clientFd)
{
	//tell other clients who left the gp chat
	sprintf(sendBuffer, "server: client %d just left\n", client[pollIdx].id);
	broadcastMsg(clientFd);
	close (clientFd);

	// fill in the blank spot made by client that left the gp
	// upon both pollfds & client
	for (int i = pollIdx; i < fdCount; ++i)
	{
		pollfds[i] = pollfds[i + 1];
		client[i] = client[i + 1];
	}
	--fdCount;
}

void processMsg(int recvRet, int pollIdx, int clientFd)
{
	// loop - replace char by char
	for (int i = 0, j = strlen(client[pollIdx].msg);
		 i < recvRet;
		 ++i, ++j)
	{
		if (j > RECV_BUFFER_SIZE)
			j = 0;

		client[pollIdx].msg[j] = recvBuffer[i];
		if (client[pollIdx].msg[j] == '\n')
		{
			client[pollIdx].msg[j] = '\0';
			sprintf(sendBuffer, "client %d: %s\n", client[pollIdx].id, client[pollIdx].msg);
			broadcastMsg(clientFd);
			bzero(client[pollIdx].msg, RECV_BUFFER_SIZE);
			j = -1;
		} 
	} 
}