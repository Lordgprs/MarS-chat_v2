#include "chat_mgr.h"
#include <string>
#include <fstream>

// construct
ChatMgr::ChatMgr() {
	std::cout <<
		"Welcome to the chat,\n"
		"to view help, type /help"
		<< std::endl;
}

// function help
void ChatMgr::displayHelp() {
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

// login availability
bool ChatMgr::isLoginAvailable(const std::string& login) const {
	return users_.find(login) == users_.end();
}

void ChatMgr::signUp() {
	if (loggedUser_ != nullptr) {
		std::cout << "to register, enter /logout.\n" << std::endl;
		return;
	}

	std::string name, login, password;

	std::cout << "Enter login: ";
	std::getline(std::cin, login);

	std::cout << "Enter password: ";
	std::getline(std::cin, password);

	std::cout << "Enter name: ";
	std::getline(std::cin, name);

	if (!isLoginAvailable(login)) {
		throw std::invalid_argument("The login you entered is already in use.");
	}

	if (login.empty() || password.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Login or password cannot be empty.");
	}

	if (!isValidLogin(login)) {
		// invalid argument passed
		throw std::invalid_argument("Login contains invalid characters.");
	}

	users_.emplace(login, ChatUser(login, password, name));
	std::cout << "User registered successfully.\n" << std::endl;
}

bool ChatMgr::isValidLogin(const std::string& login) const {
	// allowed characters, verification
	for (const char c : login) {
		if (!std::isalnum(c) && c != '-' && c != '_') {
			return false;
		}
	}
	return true;
}

void ChatMgr::signIn() {
	if (loggedUser_ != nullptr) {
		std::cout << "For log in you must sign out first. Enter '/logout' to sign out\n" << std::endl;
		return;
	}

	std::string login, password;

	std::cout << "Enter login: ";
	getline(std::cin, login);
	std::cout << "Enter password: ";
	getline(std::cin, password);

	auto it = users_.find(login);
	if (it == users_.end() || 
		it->second.getLogin() != login || 
		it->second.getPassword() != password) {
		// invalid argument passed
		throw std::invalid_argument("Invalid login or password");
	}

	std::cout << "Hi! " << it->second.getName() << ", welcome to the chat!\n" << std::endl;

	loggedUser_ = &it->second;
}

void ChatMgr::signOut() {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}

	loggedUser_ = nullptr;
}

void ChatMgr::removeUser(ChatUser& user) {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}
	signOut();
	users_.erase(user.getLogin());
	std::cout << "User removed successfully\n" << std::endl;
}

void ChatMgr::sendMessage(const std::string& message) {
	if (message.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Message cannot be empty");
	}

	if (message[0] == '@') {
		size_t pos = message.find(' ');
		if (pos != std::string::npos) {
			std::string receiverName = message.substr(1, pos - 1);
			std::string messageText = message.substr(pos + 1);
			sendPrivateMessage(*loggedUser_, receiverName, messageText);
		}
	}
	else {
		sendBroadcastMessage(*loggedUser_, message);
	}
}

void ChatMgr::sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText) {
	if (users_.find(receiverName) == users_.end()) {
		throw std::invalid_argument("User @" + receiverName + " does not exist");
	}
	messages_.emplace_back(std::make_shared<PrivateMessage>(sender.getLogin(), receiverName, messageText));
}

void ChatMgr::sendBroadcastMessage(ChatUser& sender, const std::string& message) {
	// Dynamically allocate memory for new message
	messages_.emplace_back(std::make_shared<BroadcastMessage>(sender.getLogin(), message, users_));
}

void ChatMgr::work() {
	// stores user input
	std::string input_text;

	while (true) {
		try {
			// working out the program algor5ithm
			std::cout << (loggedUser_ ? loggedUser_->getLogin() : "") << "> ";
			getline(std::cin, input_text);

			if (input_text == "/help") {
				// output help
				displayHelp();
			}
			else if (input_text == "/signup") {
				// registration
				signUp();
			}
			else if (input_text == "/signin") {
				// authorization
				signIn();
			}
			else if (input_text == "/logout") {
				// logout
				signOut();
			}
			else if (input_text == "/remove") {
				// removing current user
				removeUser(*loggedUser_);
			}
			else if (input_text == "/exit") {
				// closing the program
				break;
			}
			else if (loggedUser_ != nullptr) {
				// if there is an authorized user, we send a message
				sendMessage(input_text);
			}
			else {
				std::cout << 
					"the command is not recognized, \n"
					"to output help, type /help\n" 
				<< std::endl;
			}
			if (loggedUser_ != nullptr) {
				checkUnreadMessages();
			}
		}
		catch (std::invalid_argument e) {
			// exception handling
			std::cout << "Error: " << e.what() << "\n" << std::endl;
		}
	}
}

void ChatMgr::checkUnreadMessages() {
	for (int i = 0; i < messages_.size(); ++i) {
		messages_[i]->printIfUnreadByUser(loggedUser_->getLogin());
	}
}
