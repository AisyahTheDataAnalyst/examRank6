// 1. HEADERS
// from main.c
#include <string.h>		// strlen
#include <unistd.h>		// write, close
#include <sys/socket.h>	// socket, bind, listen, accept, send, recv 
#include <netinet/in.h>	// AF_INET == IPv4
// me add
#include <stdio.h>		// sprintf
#include <stdlib.h>		// exit, atoi
#include <poll.h>		// poll
#include <strings.h>	// bzero

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

// GLOBAL VARS
struct pollfd pollfds[MAX_CLIENT + 1];
t_client client[MAX_CLIENT + 1];
int fdCount = 0;
int currId = 0;
char sendBuff[SEND_BUFFER_SIZE];
char recvBuff[RECV_BUFFER_SIZE];



// 3. LIST OF FN HELPERS
void fatalError_exit();
void newConnection(int);
void broadcastMsg(int);
void handleClient(int, int);
void leftChat(int, int);
void processMsg(int, int, int);


// 4. INT MAIN
int main (int ac, char **av)
{
	if (ac != 2)
	{
		char *str = "Wrong number of arguments\n";
		write(2, str, strlen(str));
		exit(1); // explicitely stated in subject to exit(1) for argument number's inaccuracy
	}

	// ====================
	// copied from main.c
	// ====================
	int sockfd;
	struct sockaddr_in servaddr; 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) 
		fatalError_exit();
	bzero(&servaddr, sizeof(servaddr)); 
	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); // replace 8081 with atoi(av[1])
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) 
		fatalError_exit();
	if (listen(sockfd, 10) != 0) 
		fatalError_exit();
	// ======================

	pollfds[0].fd = sockfd;
	pollfds[0].events = POLLIN;
	++fdCount;

	while (1)
	{
		if (poll(pollfds, fdCount, -1) == -1) //! -1 in poll - for what?
			fatalError_exit();
		if (pollfds[0].revents & POLLIN) // new clients into gp chat!
			newConnection(sockfd);
		for (int i = 1; i < fdCount; ++i)
			if (pollfds[i].revents & POLLIN) // clients sending msgs / left chat
				handleClient(pollfds[i].fd, i);
	}
	return 0;
}


// ===============
// 5. HELPER FNs
// ===============

void fatalError_exit()
{
	char *str = "Fatal error\n";
	write(2, str, strlen(str));
	exit(1);
}

void newConnection(int sockfd)
{
	// setting up accepting new connection
		// from main.c
	struct sockaddr_in cli; 
	socklen_t len = sizeof(cli);
	int connfd = accept(sockfd, (struct sockaddr *)&cli, &len); 
	if (connfd < 0)
		//
		fatalError_exit();
	if (fdCount > MAX_CLIENT)
	{
		close(connfd);
		return ;
	}

	// adding into pollfds & fdCount
	int currFdCount = fdCount++;
	pollfds[currFdCount].fd = connfd;
	pollfds[currFdCount].events = POLLIN;

	// handle client id sequence
	client[currFdCount].id = currId++;

	// broadcast new client arrived to the group chat
	bzero(client[currFdCount].msg, RECV_BUFFER_SIZE);
	sprintf(sendBuff, "server: client %d just arrived\n", client[currFdCount].id);
	broadcastMsg(connfd);
}

void broadcastMsg(int senderfd)
{
	for(int i = 1; i < fdCount; ++i)
		if (pollfds[i].fd != senderfd && pollfds[i].fd != -1)
			send(pollfds[i].fd, sendBuff, strlen(sendBuff), 0); //! 0 - for what?
}

void handleClient(int clientFd, int pollidx)
{
	int recvRet = recv(clientFd, recvBuff, RECV_BUFFER_SIZE, 0); //! 0 - for what?
	if (recvRet <= 0)
		leftChat(clientFd, pollidx);
	else
		processMsg(pollidx, recvRet, clientFd);
}

void leftChat(int clientFd, int pollidx)
{
	sprintf(sendBuff, "server: client %d just left\n", client[pollidx].id);
	broadcastMsg(clientFd); // telling everyone who left the gp chat
	close(clientFd);

	// rest of the next clients in the poll array move to the left side
	// fill in the empty space by the client that left the chat
	for (int i = pollidx; pollidx < fdCount; ++i)
	{
		pollfds[i] = pollfds[i + 1];
		client[i] = client[i + 1];
	}
	// officially lesser clients now
	--fdCount;
}

void processMsg(int pollidx, int recvRet, int clientFd)
{
	for (int i = 0, j = strlen(client[pollidx].msg);
		 i < recvRet;
		 ++i, ++j)
	{
		if (j > RECV_BUFFER_SIZE)
			j = 0; // overwrite the extra long chars back to the front char
		
		client[pollidx].msg[j] = recvBuff[i]; // replace char by char of recvBuff into client[].msj
		if (client[pollidx].msg[j] == '\n') // if the replaced char is newline character
		{
			client[pollidx].msg[j] = '\0'; // replace original \n with \0
			sprintf(sendBuff, "client %d: %s\n", client[pollidx].id, client[pollidx].msg);
			broadcastMsg(clientFd);
			bzero(client[pollidx].msg, RECV_BUFFER_SIZE);
			j = -1; /// so that it resets (j = 0) after ++j
		}
	}
}