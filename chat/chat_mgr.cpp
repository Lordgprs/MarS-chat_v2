#include "chat_mgr.h"
#include <string>

// construct
chat_mgr::chat_mgr() {
	std::cout << "Welcome to the chat," << std::endl;
	std::cout << "to output help, type /help" << std::endl;
}

// function help
void chat_mgr::displayHelp() {
	std::cout << "Available commands:" << std::endl;
	std::cout << " /help - chat help, displays a list of commands to manage the chat" << std::endl;
	std::cout << " /signup - registration, user enters data for registration" << std::endl;
	std::cout << " /signin - authorization, only a registered user can authorize" << std::endl;
	std::cout << " /logout - user logout" << std::endl;
	std::cout << " /remove - delete registered user" << std::endl;
	std::cout << " /exit - close the program" << std::endl;
	std::cout << " @username is an addressable message," << std::endl;
	std::cout << "   not an addressable message for all users.\n" << std::endl;
}

// login availability
bool chat_mgr::isLoginAvailable(const std::string& login) const {
	return users_.find(login) == users_.end();
}

void chat_mgr::signUp() {
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

	users_.emplace(login, chat_user(login, password, name));
	std::cout << "User registered successfully.\n" << std::endl;
}

bool chat_mgr::isValidLogin(const std::string& login) const {
	// allowed characters, verification
	for (char c : login) {
		if (!std::isalnum(c) && c != '-' && c != '_') {
			return false;
		}
	}
	return true;
}

chat_user *chat_mgr::signIn() {
	if (loggedUser_ != nullptr) {
		std::cout << "to log in, enter /logout.\n" << std::endl;
		return nullptr;
	}

	std::string login, password;

	std::cout << "Enter login: ";
	getline(std::cin, login);
	std::cout << "Enter password: ";
	getline(std::cin, password);

	auto id = users_.find(login);
	if (id == users_.end() || id->second.isAuthorized() || id->second.getLogin() != login || id->second.getPassword() != password) {
		// invalid argument passed
		throw std::invalid_argument("Invalid login or password.");
	}

	std::cout << "Hi! " << id->second.getName() << " welcome to the chat!" << std::endl;

	id->second.setAuthorized(true);
	return &id->second;
}

void chat_mgr::signOut() {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in.\n" << std::endl;
		return;
	}
	loggedUser_->setAuthorized(false);
	loggedUser_ = nullptr;
}

void chat_mgr::removeUser(chat_user& user) {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in.\n" << std::endl;
		return;
	}
	signOut();
	users_.erase(user.getLogin());
	std::cout << "User removed successfully.\n" << std::endl;
}

void chat_mgr::sendMessage(const std::string& message) {
	if (message.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Message cannot be empty.");
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

void chat_mgr::sendPrivateMessage(chat_user& sender, const std::string& receiverName, const std::string& messageText) {
	messages_.emplace_back(std::make_shared<private_message>(sender.getLogin(), receiverName, messageText));
}

void chat_mgr::sendBroadcastMessage(chat_user& sender, const std::string& message) {
	messages_.emplace_back(std::make_shared<broadcast_message>(sender.getLogin(), message, users_));
}

void chat_mgr::work() {
	// stores user input
	std::string input_text;

	while (true) {
		try {
			// working out the program algorithm
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
				loggedUser_ = signIn();
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
		catch (const std::exception e) {
			// exception handling
			std::cout << "Error: " << e.what() << "\n" << std::endl;
		}
	}
}

void chat_mgr::checkUnreadMessages() {
	for (auto &message : messages_) {
		message->printIfUnreadByUser(loggedUser_->getLogin());
	}
}
