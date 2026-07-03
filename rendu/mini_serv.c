// thank you mai <3

// 1. HEADERS
#include <stdlib.h>		// exit, atoi
#include <stdio.h>		// sprintf
#include <poll.h>		// poll
#include <strings.h>	// bzero
// #include <sys/types.h>	//
// === copied from main.c ===
#include <string.h>		// strlen
#include <unistd.h>		// write, close
#include <sys/socket.h>	// socket, accept, listen, send, recv, bind
#include <netinet/in.h>	// AF_INET (macro for sockaddr_in)



// 2. MACROS, STRUCTS, GLOBAL VARIABLES
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

// GLOBAL VARS
struct pollfd pollfds[MAX_CLIENT + 1];	// plus 1 to include server
t_client client[MAX_CLIENT + 1];	// plus 1 to include server
int fdCount = 0;
int currId = 0;
char sendBuffer[SEND_BUFFER_SIZE];
char recvBuffer[RECV_BUFFER_SIZE];



// 3. LIST OF ALL HELPER FUNCTIONS
void fatalError_exit();
void boardcastMsg(int senderfd);
void newConnection(int sockfd);
void disconnection(int fd, int idx);
void processMsg(int idx, int ret);
void handleClient(int fd, int idx);



// 4. INT MAIN
int main(int argc, char *argv[]) 
{
	if (argc != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1); // explicitely stated in subject to exit(1) for argument number's inaccuracy
	}

	// =====================================
	// === copied from main.c's int main ===
		// => original
		// int sockfd, connfd, len; 
		// struct sockaddr_in servaddr, cli; 
		// => edited, others are in newConnection()
	int sockfd;
	struct sockaddr_in servaddr;
		// => done edited
	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) 
		fatalError_exit();
	bzero(&servaddr, sizeof(servaddr));
	// assign IP, PORT 
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1]));
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0)
		fatalError_exit();
	if (listen(sockfd, 10) != 0)
		fatalError_exit();
	// === end copied ===
	// =====================================

	// add server into pollfds as the listener
	// pollfds[0] == listener
	// pollfds => all listener & clients
	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;
	fdCount++;

	while (1)
	{
		if (poll(pollfds, fdCount, -1) == -1)	// poll malfunction, //! -1 in poll - for what?
			fatalError_exit();
		if (pollfds[0].revents & POLLIN)		// new clients getting into the group chat!
			newConnection(sockfd);
		for (int i = 1; i < fdCount; ++i)
			if (pollfds[i].revents & POLLIN)	// existing clients send messages / left chat!
				handleClient(pollfds[i].fd, i);
	}
	return 0;
}



// ===================
// 5. HELPER FUNCTIONS
// ===================

void fatalError_exit() 
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

void newConnection(int sockfd) 
{
	// setting up accepting new connection
		// === copied these 4 lines from main.c's int main , slightly adjusted ===
	struct sockaddr_in cli;
	socklen_t len = sizeof(cli);
	int connfd = accept(sockfd, (struct sockaddr *)&cli, &len);	// listener accepting new incoming TCP connections into becoming clients
	if (connfd < 0)
		fatalError_exit();
	if (fdCount > MAX_CLIENT) 
	{
		close(connfd);
		return ;
	}

	pollfds[fdCount].fd = connfd;
	pollfds[fdCount].events = POLLIN;

	client[fdCount].id = currId;
	currId++;

	bzero(client[fdCount].msg, RECV_BUFFER_SIZE);
	sprintf(sendBuffer,  "server: client %d just arrived\n", client[fdCount].id);
	boardcastMsg(connfd);
	fdCount++;
}

void boardcastMsg(int senderfd) 
{
	for (int i = 1; i < fdCount; ++i)
		if (pollfds[i].fd != senderfd && pollfds[i].fd != -1)
			send(pollfds[i].fd, sendBuffer, strlen(sendBuffer), 0);
}

void handleClient(int fd, int idx) 
{
	int ret = recv(fd, recvBuffer, RECV_BUFFER_SIZE, 0);
	if (ret <= 0)
		disconnection(fd, idx);
	else
		processMsg(idx, ret);
}

void disconnection(int fd, int idx) 
{
	sprintf(sendBuffer, "server: client %d just left\n", client[idx].id);
	boardcastMsg(fd);
	close (fd);

	for (int i = idx; i < fdCount - 1; ++i)
	{
		pollfds[i] = pollfds[i + 1];
		client[i] = client[i + 1];
	}
	--fdCount;
}

void processMsg(int idx, int ret) 
{
	for (int i = 0, j = strlen(client[idx].msg);
		i < ret; 
		++i, ++j)
	{
		if (j > RECV_BUFFER_SIZE)
			j = 0;

		client[idx].msg[j] = recvBuffer[i];
		if (client[idx].msg[j] == '\n') 
		{
			client[idx].msg[j] = '\0';
			sprintf(sendBuffer, "client %d: %s\n", client[idx].id, client[idx].msg);
			boardcastMsg(pollfds[idx].fd);
			bzero(client[idx].msg, RECV_BUFFER_SIZE);
			j = -1;
		}
	}
}
