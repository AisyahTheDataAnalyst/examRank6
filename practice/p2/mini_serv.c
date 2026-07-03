// 1. HEADERS
// me add
#include <poll.h>		// poll
#include <stdlib.h>		// atoi, exit
#include <strings.h>	// bzero
#include <stdio.h>		// sprintf
// main.c
#include <string.h>		// strlen
#include <unistd.h>		// write, close
#include <sys/socket.h>	// socket, listen, bind, accept, send, recv
#include <netinet/in.h> // AF_INET

// 2. MACROS, STRUCT, GLOBAL VARS
// MACROS
enum
{
	RECV_BUFFER_SIZE = 100000,
	SEND_BUBBER_SIZE = 100050,
	MAX_CLIENT = 1024
};

// STRUCT
typedef struct s_client
{
	int id;
	char msg[RECV_BUFFER_SIZE];
}	t_client;

// GLOBAL VARS
struct pollfd pollfds[MAX_CLIENT + 1]; //including server
t_client client[MAX_CLIENT + 1]; //including server
int fdCount = 0;
int currId = 0;
char sendBuffer[SEND_BUBBER_SIZE];
char recvBuffer[RECV_BUFFER_SIZE];


// 3. LIST HELPER FN - GOT 6
void fatalError_exit();
void newConnection(int);
void boardcastMsg(int);
void handleClient(int, int);
void leftGpChat(int, int);
void processMsg(int, int, int);

// 4. INT MAIN
int main(int ac, char **av)
{
	if (ac != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1); 
	}

	int sockfd;
	struct sockaddr_in servaddr;
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

	// include listener into pollfd as pollfds[0]
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;
	++fdCount;

	while (1)
	{
		// 1. poll guard 
		if (poll(pollfds, fdCount, -1) == -1)
			fatalError_exit();
		// 2. new TCP connection
		if (pollfds[0].revents & POLLIN)
			newConnection(sockfd);
		// 3. clients did something something~
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
	struct sockaddr_in cli;
	int connfd;
	socklen_t len;

	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0)
		fatalError_exit();
	if (fdCount > MAX_CLIENT)
	{
		close(connfd);
		return;
	}

	// adding fdCount & pollfds
	int currFdCount = fdCount++;
	pollfds[currFdCount].fd = connfd;
	pollfds[currFdCount].events = POLLIN;

	// adding clientId
	client[currFdCount].id = currId++;

	//  tell other clients who join the gp chat
	bzero(client[currFdCount].msg, RECV_BUFFER_SIZE);
	sprintf(sendBuffer, "server: client %d just arrived\n", client[currFdCount].id);
	boardcastMsg(connfd);
}

void boardcastMsg(int clientFd)
{
	for (int i = 1; i < fdCount; ++i)
		if (pollfds[i].fd != clientFd && pollfds[clientFd].fd != -1)
			send(pollfds[i].fd, sendBuffer, strlen(sendBuffer), 0);
}

void handleClient(int clientFd, int pollIdx)
{
	// 1. go thru recv
	ssize_t recvRet = recv(clientFd, recvBuffer, RECV_BUFFER_SIZE, 0);
	if (recvRet <= 0)
		leftGpChat(clientFd, pollIdx);
	else
		processMsg(pollIdx, recvRet, clientFd); 
}

void leftGpChat(int clientFd, int pollIdx)
{
	// tell other clients that someone has left gp chat
	sprintf(sendBuffer, "server: client %d just left\n", client[pollIdx].id);
	boardcastMsg(clientFd);
	
	// clientFd not used anymore
	close(clientFd);

	// make other clients fill in the blank spot by client that left chat
	// fill in for pollfds & client
	for (int i = pollIdx; i < fdCount; ++i)
	{
		pollfds[i] = pollfds[i + 1];
		client[i] = client[i + 1];
	}
	--fdCount;
}

void processMsg(int pollIdx, int recvRet, int clientFd)
{
	// loop through each char
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
			boardcastMsg(clientFd);
			bzero(client[pollIdx].msg, RECV_BUFFER_SIZE);
			j = -1;
		}
	}
}





