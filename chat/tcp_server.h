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
	void work();

private:
	const unsigned short MESSAGE_LENGTH{ 1024 };
	const int BACKLOG{ 5 };
	sockaddr_in server_;
	sockaddr_in client_;

	int sockFd_;
	char message_[MESSAGE_LENGTH];
};

