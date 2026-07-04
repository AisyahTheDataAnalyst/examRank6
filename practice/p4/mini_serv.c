// 1. HEADERS
// from main.c
#include <string.h>		// strlen
#include <unistd.h>		// write, close
#include <sys/socket.h>	// socket, listen, bind, accept, send, recv
#include <netinet/in.h>	// AF_INET
// from me
#include <stdio.h>		// sprintf
#include <strings.h>	// bzero
#include <poll.h>		// poll
#include <stdlib.h>		// atoi, exit

// 2. MACROS, STRUCT, GLOBAL VARS
// MACROS
enum
{
	RECV_BUFFER_SIZE = 100000,
	SEND_BUFFER_SIZE = 100050,
	MAX_CLIENT = 1024	
};

// STRUCT
typedef struct s_client
{
	int id;
	char msg[RECV_BUFFER_SIZE];
}	t_client;

// GLOBAL VARS - got 6
struct pollfd pollfds[MAX_CLIENT + 1];
t_client client[MAX_CLIENT + 1];
char sendBuffer[SEND_BUFFER_SIZE];
char recvBuffer[RECV_BUFFER_SIZE];
int fdCount = 0;
int currId = 0;


// 3. LIST ALL HELPER FNs - GOT 6
void fatalError_exit();
void newConnection(int);
void broadcastMsg(int);
void handleClient(int, int);
void leftGpChat(int, int);
void processMsg(int, int, int);

// 4. INT MAIN
int main(int ac, char **av)
{
	// 1. arguments handling
	if (ac != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1);
	}

		// 2. from main.c
	int sockfd; //, connfd, len;
	struct sockaddr_in servaddr; //, cli; 
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
		// finish copy from main.c

	
	// 3. register listener into pollfds & fdCount
	//! HARDCODE THIS TO POLLFDS[0] SPECIFICALLY FOR LISTENER
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;
	fdCount++;

	// WRONG => LISTENER / SERVER IS NOT CLIENT
	// // 4. update new client id
	// client[currFdCount].id = currId++;

	// 5. the loop!
	while (1)
	{
		// poll guard
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
		// 1. from main.c - setup new connection
	int connfd;
	socklen_t len;
	struct sockaddr_in cli; 
	
	len = sizeof(cli);
	connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
	if (connfd < 0)
		// end copy from main.c
		fatalError_exit();
	if (fdCount > MAX_CLIENT)
	{
		close(connfd);
		return;
	}

	// 2. register client into fdCOunt & pollfds
	int currFdCount = fdCount++;
	pollfds[currFdCount].fd = connfd;
	pollfds[currFdCount].events = POLLIN;

	// 3. update client & cuurId
	client[currFdCount].id = currId++;

	// 4. broadcast new comer to other clients in gp chat!
	// bzero(client[currFdCount].msg, SEND_BUFFER_SIZE); //!=> WRONG
	bzero(sendBuffer, SEND_BUFFER_SIZE);
	sprintf(sendBuffer, "server: client %d just arrived\n", client[currFdCount].id);
	// broadcastMsg(pollfds[currFdCount].fd); //! => WRONG
	broadcastMsg(connfd);
}

void broadcastMsg(int clientFd)
{
	for (int i = 1; i < fdCount; ++i)
		if (pollfds[i].fd != clientFd && pollfds[i].fd != -1)
			// send(pollfds[i].fd, sendBuffer, SEND_BUFFER_SIZE, 0); //! => WRONG
			send(pollfds[i].fd, sendBuffer, strlen(sendBuffer), 0); 
}

void handleClient(int clientFd, int pollIdx)
{
	// 1. do recv
	// ssize_t recvRet = recv(clientFd, sendBuffer, SEND_BUFFER_SIZE, 0); //!=> WRONG
	ssize_t recvRet = recv(clientFd, recvBuffer, RECV_BUFFER_SIZE, 0);
	if (recvRet <= 0)
		leftGpChat(clientFd, pollIdx);
	else
		processMsg(recvRet, pollIdx, clientFd);
}

void leftGpChat(int clientFd, int pollIdx)
{
	//! => FORGOT
	// broadcast to others who left the gp chat
	sprintf(sendBuffer, "server: client %d just left\n", client[pollIdx].id);
	broadcastMsg(clientFd);
	// close fd
	close(clientFd);

	// move to the left to fill in the black caused by client that left the chat gp
	// for both pollfds & client
	for (int i = pollIdx; i < fdCount; ++i)
	{
		pollfds[i] = pollfds[i + 1];
		client[i] = client[i + 1];
	}
	--fdCount;
}

void processMsg(int recvRet, int pollIdx, int clientFd)
{
	// replace char by char in a loop!
	for (int i = 0, j = strlen(client[pollIdx].msg);
		 i < recvRet;
		 ++i, ++j)
	{
		// check if recvBuffer over the fixed limit
		if (j > RECV_BUFFER_SIZE)
			j = 0;

		client[pollIdx].msg[j] = recvBuffer[i];
		if (client[pollIdx].msg[j] == '\n')
		{
			client[pollIdx].msg[j] = '\0';
			sprintf(sendBuffer, "client %d: %s\n", client[pollIdx].id, client[pollIdx].msg);
			broadcastMsg(clientFd);
			bzero(client[pollIdx].msg, RECV_BUFFER_SIZE);
			//! => FORGOT
			j = -1;
		}
	}
}
