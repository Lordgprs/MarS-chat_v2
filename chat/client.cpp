#include "chat_client.h"

int main() {
	try {
		ChatClient chat;
		chat.work();
	}
	catch (const std::runtime_error &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return EXIT_SUCCESS;
}
