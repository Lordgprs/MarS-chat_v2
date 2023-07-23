#include "chat_mgr.h"

// construct
chat_mgr::chat_mgr() {}

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
bool chat_mgr::isLoginAvailable(const std::string& login) const
{
	return users.find(login) == users.end();
}

void chat_mgr::signUp(const std::string& login, const std::string& password, const std::string& name)
{
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

	users.emplace(login, chat_user(login, password, name));
}

bool chat_mgr::isValidLogin(const std::string& login) const
{
	// allowed characters, verification
	for (char c : login) {
		if (!std::isalnum(c) && c != '-' && c != '_') {
			return false;
		}
	}
	return true;
}

chat_user& chat_mgr::signIn(const std::string& login, const std::string& password)
{
	auto id = users.find(login);
	if (id == users.end() || id->second.isAuthorized() || id->second.getLogin() != login || id->second.getPassword() != password) {
		// invalid argument passed
		throw std::invalid_argument("Invalid login or password.");
	}

	std::cout << "Hi! " << id->second.getName() << " welcome to the chat!" << std::endl;

	id->second.setAuthorized(true);
	return id->second;
}

void chat_mgr::signOut(chat_user& user)
{
	user.setAuthorized(false);
}

void chat_mgr::removeUser(chat_user& user)
{
	users.erase(user.getLogin());
	user.setAuthorized(false);
}

void chat_mgr::sendMessage(chat_user& sender, const std::string& message) const
{
	if (!sender.isAuthorized()) {
		// runtime error
		throw std::runtime_error("User must be authorized to send a message.");
	}

	if (message.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Message cannot be empty.");
	}

	if (message[0] == '@') {
		size_t pos = message.find(' ');
		if (pos != std::string::npos) {
			std::string receiverName = message.substr(1, pos - 1);
			std::string messageText = message.substr(pos + 1);
			sendPrivateMessage(sender, receiverName, messageText);
		}
	}
	else {
		sendBroadcastMessage(sender, message);
	}
}

void chat_mgr::sendPrivateMessage(chat_user& sender, const std::string& receiverName, const std::string& messageText) const
{
	auto id = std::find_if(users.begin(), users.end(), [&receiverName](const auto& entry) {
		return entry.second.getLogin() == receiverName;
		});

	if (id != users.end()) {
		std::cout << sender.getLogin() << " to " << receiverName << ": " << messageText << std::endl;
	}
	else {
		// runtime error
		throw std::runtime_error("The specified user does not exist or is not online.");
	}
}

void chat_mgr::sendBroadcastMessage(chat_user& sender, const std::string& message) const
{
	for (const auto& entry : users) {
		const chat_user& receiver = entry.second;
		if (receiver.isAuthorized()) {
			std::cout << sender.getLogin() << " to all: " << message << std::endl;
		}
	}
}