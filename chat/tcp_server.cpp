#include "tcp_server.h"

#include <stdexcept>

TcpServer::TcpServer(unsigned short listen_port) {
	sockFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockFd == -1) {
		throw std::runtime_error{ "Error while creating socket!" };
	}

	server_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_.sin_port = htons(listen_port);
	server_.sin_family = AF_INET;

	auto bindStatus = bind(sockFd_, static_cast<sockaddr *>(&server_), sizeof(server_));
	if (bindStatus == -1) {
		throw std::runtime_error{ "Error: socket binding failed" };
	}

	auto connectionStatus = listen(sockFd, BACKLOG);
	if (connectionStatus == -1) {
		throw std::runtime_error{ "Error: could not listen the specified TCP port" };
	}
}

TcpServer::~TcpServer() {
	close(sockFd_);
}

TcpServer::work() {
	auto length = sizeof(client_);
	auto connection = accept(sockFd_, static_cast<sockaddr *>(&client_), &length);
	while (true) {
		bzero(message_, MESSAGE_LENGTH);
		read(connection, message_, MESSAGE_LENGTH);
		if (strncmp(message_, "/quit", 5) == 0 || strncmp(message_, "/exit", 5) == 0) {
			break;
		}

		bzero(message_, MESSAGE_LENGTH);
		std::cin >> message_;
	}
}
