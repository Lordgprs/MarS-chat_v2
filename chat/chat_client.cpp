#include "chat_client.h"
#include "project_lib.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#if defined(__linux__)
#include <sys/utsname.h>
#elif defined(_WIN64) or defined(_WIN32)
#pragma comment(lib, "ntdll")

extern "C" NTSTATUS __stdcall RtlGetVersion(OSVERSIONINFOEXW * lpVersionInformation);
#endif

namespace fs = std::filesystem;

// construct
ChatClient::ChatClient() {
	printSystemInformation();

	std::cout <<
		"Welcome to the chat,\n"
		"to view help, type /help" << std::endl;
	std::cout << "Connecting to " << config_["ServerAddress"] << ':' << config_["ServerPort"] << "..." << std::endl;

	server_.sin_addr.s_addr = inet_addr(config_["ServerAddress"].c_str());
	server_.sin_port = htons(stoi(config_["ServerPort"]));
	server_.sin_family = AF_INET;

	sockFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockFd_ == -1) {
		throw std::runtime_error{ "Error while creating socket" };
	}

	auto connection = connect(sockFd_, reinterpret_cast<sockaddr *>(&server_), sizeof(server_));
	if (connection == -1) {
		throw std::runtime_error{ "Could not connect to server..." };
	}

}

ChatClient::~ChatClient() {
	close(sockFd_);
}


// login availability
bool ChatClient::isLoginAvailable(const std::string& login) const {
	return users_.find(login) == users_.end();
}

void ChatClient::signUp() {
	if (loggedUser_ != nullptr) {
		std::cout << "to register, enter /logout.\n" << std::endl;
		return;
	}

	std::string name, login, password, hash;

	std::cout << "Enter login: ";
	std::getline(std::cin, login);

	std::cout << "Enter password: ";
	std::getline(std::cin, password);

	std::cout << "Enter name: ";
	std::getline(std::cin, name);

	if (!isLoginAvailable(login)) {
		throw std::invalid_argument("The login you entered has been already registered.");
	}

	if (login.empty() || password.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Login and password cannot be empty.");
	}

	if (!isValidLogin(login)) {
		// invalid argument passed
		throw std::invalid_argument("Login contains invalid characters.");
	}
	
	/* SHA256 sha;
	sha.update(password);
	uint8_t *digest = sha.digest();
	hash = SHA256::toString(digest);
	delete[] digest;

	users_.emplace(login, ChatUser(login, hash, name));
	try {
		users_.at(login).save(USER_CONFIG);
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}
	std::cout << "User registered successfully.\n" << std::endl; */
}

bool ChatClient::isValidLogin(const std::string& login) const {
	// allowed characters, verification
	for (const char c : login) {
		if (!std::isalnum(c) && c != '-' && c != '_') {
			return false;
		}
	}
	return true;
}

void ChatClient::signIn() {
	if (loggedUser_ != nullptr) {
		std::cout << "For log in you must sign out first. Enter '/logout' to sign out\n" << std::endl;
		return;
	}

	std::string login, password, hash;

	std::cout << "Enter login: ";
	getline(std::cin, login);
	std::cout << "Enter password: ";
	getline(std::cin, password);
	std::string cmd{ std::string{"/signin:"} + login + ":" + password };
	strcpy(message_, cmd.c_str());
	sendMessage();
}

void ChatClient::signOut() {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}

	loggedUser_ = nullptr;
}

void ChatClient::removeUser(ChatUser& user) {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}
	signOut();
	users_.erase(user.getLogin());
	usersFileMustBeUpdated_ = true;
	std::cout << "User removed successfully\n" << std::endl;
}

ssize_t ChatClient::sendMessage() const {
	if (*message_ == '\0') {
		// invalid argument passed
		throw std::invalid_argument("Message cannot be empty");
	}
	ssize_t bytes = write(sockFd_, message_, MESSAGE_LENGTH);

	return bytes;
}

void ChatClient::sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText) {
	if (users_.find(receiverName) == users_.end()) {
		throw std::invalid_argument("User @" + receiverName + " does not exist");
	}
	messages_.emplace_back(std::make_shared<PrivateMessage>(sender.getLogin(), receiverName, messageText));
}

void ChatClient::sendBroadcastMessage(ChatUser& sender, const std::string& message) {
	// Dynamically allocate memory for new message
	messages_.emplace_back(std::make_shared<BroadcastMessage>(sender.getLogin(), message, users_));
}

void ChatClient::work() {
	while (true) {
		try {
			std::fill(message_, message_ + MESSAGE_LENGTH, '\0');
			std::cout << (loggedUser_ ? loggedUser_->getLogin() : "") << "> ";
			std::cin.getline(message_, MESSAGE_LENGTH);
			
			// working out the program algor5ithm

			if (strncmp(message_, "/help", 5) == 0) {
				// output help
				Chat::displayHelp();
			}
			else if (strncmp(message_, "/signup", 7) == 0) {
				// registration
				signUp();
			}
			else if (strncmp(message_, "/signin", 7) == 0) {
				// authorization
				signIn();
			}
			else if (strncmp(message_, "/logout", 7) == 0) {
				// logout
				signOut();
			}
			else if (strncmp(message_, "/remove", 7) == 0) {
				// removing current user
				if (loggedUser_) {
					removeUser(*loggedUser_);
				}
			}
			else if (loggedUser_) {
				sendMessage();
			}
			else if (
				strncmp(message_, "/exit", 5) == 0 ||
				strncmp(message_, "/quit", 5) == 0) {
				// closing the program
				break;
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

void ChatClient::checkUnreadMessages() {
	for (int i = 0; i < messages_.size(); ++i) {
		messages_[i]->printIfUnreadByUser(loggedUser_->getLogin());
	}
}

void ChatClient::saveUsers() const {
	std::cout << "Saving users information to file " << USER_CONFIG << "..." << std::endl;
	std::ofstream file(USER_CONFIG, std::ios::out | std::ios::trunc);
	if (!file.is_open()) {
		throw std::runtime_error{ "Cannot open file " + USER_CONFIG + " for write" };
	}
	file.close();

	for (auto it = users_.begin(); it != users_.end(); ++it) {
		it->second.save(USER_CONFIG);		
	}
}

void ChatClient::saveMessages() const {	
	std::cout << "Saving chat history to file " << MESSAGES_LOG << "..." << std::endl;
	std::ofstream file(MESSAGES_LOG, std::ios::out | std::ios::trunc);
	if (!file.is_open()) {
		throw std::runtime_error{ "Cannot open file " + MESSAGES_LOG + " for write" };
	}
	file.close();

	for (const auto &msg: messages_) {
		msg->save(MESSAGES_LOG);		
	}
}

void ChatClient::loadUsers() {
	std::ifstream file(USER_CONFIG, std::ios::in);
	if (!file.is_open()) {
		throw std::runtime_error{ "Error: cannot open file " + USER_CONFIG + " for read" };
	}

	while (!file.eof()) {
		std::string login;
		std::string password;
		std::string name;
		getline(file, login);
		getline(file, password);
		getline(file, name);
		if (!login.empty()) {
			users_.emplace(login, ChatUser(login, password, name));
		}
	}
	file.close();
}

void ChatClient::loadMessages() {
	std::ifstream file(MESSAGES_LOG, std::ios::in);
	if (!file.is_open()) {
		throw std::runtime_error{ "Error: cannot open file " + MESSAGES_LOG + " for read" };
	}
	while (!file.eof()) {
		std::string message_type;
		std::string sender;
		std::string text;

		getline(file, message_type);
		getline(file, sender);
		if (message_type == "BROADCAST") {
			std::string users_unread_str;
			
			getline(file, users_unread_str);
			getline(file, text);

			messages_.emplace_back(std::make_shared<BroadcastMessage>(sender, text, users_, users_unread_str));
		}
		else if (message_type == "PRIVATE") {
			std::string receiver;
			std::string is_read_str;
			bool is_read;

			getline(file, receiver);
			getline(file, is_read_str);
			is_read = (is_read_str == "READ");
			getline(file, text);

			messages_.emplace_back(std::make_shared<PrivateMessage>(sender, receiver, text, is_read));
		}
	}
	file.close();
}

void ChatClient::printSystemInformation() const {
#if defined(__linux__)
	utsname uts;
	uname(&uts);

	auto pid = getpid();

	std::cout << "Current process ID: " << pid << std::endl;
	std::cout << "OS " << uts.sysname << " (" << uts.machine << ") " << uts.release << '\n' << std::endl;
#elif defined(_WIN64) or defined(_WIN32)
	OSVERSIONINFOEXW osv;
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	if (RtlGetVersion(&osv) == 0)
	{
		std::cout << "OS Windows "
			<< getLiteralOSName(osv) <<
			' ' << osv.dwMajorVersion << "."
			<< osv.dwMinorVersion << "."
			<< osv.dwBuildNumber << std::endl;
	}
	else {
		std::cout << "Unable to obtain Windows version" << std::endl;
	}
#endif
}

#if defined(_WIN64) or defined(_WIN32)
std::string ChatClient::getLiteralOSName(OSVERSIONINFOEX &osv) const {
	// returns empty string if the version is unknown
	if (osv.dwMajorVersion <= 4) {
		return "NT";
	}
	else if (osv.dwMajorVersion == 5) {
		switch (osv.dwMinorVersion) {
		case 0:
			return "2000";
			break;
		case 1:
			return "XP";
			break;
		case 2:
			return "Server 2003";
		default:
			return std::string{};
			break;
		}
	}
	else if (osv.dwMajorVersion == 6) {
		switch (osv.dwMinorVersion) {
		case 0:
			return "Vista";
			break;
		case 1:
			return "7";
			break;
		case 2:
			return "8";
			break;
		case 3:
			return "8.1";
			break;
		default:
			return std::string{};
			break;
		}
	}
	else {
		return std::to_string(osv.dwMajorVersion);
	}
}
#endif
