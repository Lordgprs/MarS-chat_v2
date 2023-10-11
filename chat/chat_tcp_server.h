#pragma once

#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>


class TcpServer {
public:
	TcpServer() = delete;
	explicit TcpServer(unsigned short listenPort);
	~TcpServer();

private:
	const unsigned short MESSAGE_LENGTH{ 1024 };
	sockaddr_in server_;
	sockaddr_in client_;

	int sockfd;
};

