#include "chat_server.h"

int main() {
	try {
		ChatServer chat;
		chat.work();
	}
	catch (const std::runtime_error &e) {
		std::cerr << "Fatal error! " << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
