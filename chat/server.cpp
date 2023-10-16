#include "chat_server.h"

ChatServer chat;

void childDeathHandler(int signum) {
	chat.childDeathHandler(signum);
}

void sigTermHandler(int signum) {
	chat.sigTermHandler(signum);
}

void sigIntHandler(int signum) {
	chat.sigIntHandler(signum);
}

int main() {
	signal(SIGCHLD, childDeathHandler);
	signal(SIGTERM, sigTermHandler);
	signal(SIGINT, sigIntHandler);
	try {
		chat.work();
	}
	catch (const std::runtime_error &e) {
		std::cerr << "Fatal error! " << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
