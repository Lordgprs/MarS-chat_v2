#include "project_lib.h"
#include <iostream>

// split string to vector
std::vector<std::string> Chat::split(const std::string &src, const std::string &delimiter) {
	size_t pos = 0;
	std::string src_copy { src };
	std::vector<std::string> result;

	while ((pos = src_copy.find(delimiter)) != std::string::npos) {
		result.emplace_back(src_copy.substr(0, pos));
		src_copy.erase(0, pos + delimiter.length());
	}
	if (!src_copy.empty()) {
		result.push_back(src_copy);
	}
	return result;
}

// function help
void Chat::displayHelp() {
	std::cout << "Available commands:\n"
		" /help - chat help, displays a list of commands to manage the chat\n"
		" /signup - registration, user enters data for registration\n"
		" /signin - authorization, only a registered user can authorize\n"
		" /logout - user logout\n"
		" /remove - delete registered user\n"
		" /exit - close the program\n"
		" Start your message with @login if you want to send a private message,\n"
		"   otherwise your message will be broadcasted to all users.\n"
		"User will receive new messages after login\n"
		<< std::endl;
}
