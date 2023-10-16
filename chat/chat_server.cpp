#include "chat_server.h"
#include "SHA256.h"
#include "project_lib.h"

#include <string>
#include <fstream>
#include <filesystem>
#if defined(__linux__)
#include <sys/utsname.h>
#include <errno.h>
#elif defined(_WIN64) or defined(_WIN32)
#pragma comment(lib, "ntdll")

extern "C" NTSTATUS __stdcall RtlGetVersion(OSVERSIONINFOEXW * lpVersionInformation);
#endif

namespace fs = std::filesystem;

// constructor
ChatServer::ChatServer() {
	mainPid_ = getpid();
	printSystemInformation();

	fs::current_path(".");
	try {
		loadUsers();
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}
	try {
		loadMessages();
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}

	std::cout <<
		"Welcome to the chat admin console. "
		"This chat server supports multiple client login and creates own process for each one.\n"
		"Type /help to view help" << std::endl;

	if(users_.empty()) {
		return;
	}

	std::cout << "Registered users: ";
	for (const auto kv: users_) {
		std::cout << '\'' << kv.first << "' ";
	}
	std::cout << '\n' << std::endl;
	
	sockFd_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockFd_ == -1) {
		throw std::runtime_error{ "Error while creating socket!" };
	}

	int trueVal = 1;
	if (setsockopt(sockFd_, SOL_SOCKET, SO_REUSEADDR, &trueVal, sizeof(trueVal)) == -1) {
		throw std::runtime_error{ std::string{ "Can not set socket options: " } + std::string{ strerror(errno) } };	
	}

	server_.sin_addr.s_addr = htonl(INADDR_ANY);
	server_.sin_port = htons(stoi(config_["ListenPort"]));
	server_.sin_family = AF_INET;

	auto bindStatus = bind(sockFd_, reinterpret_cast<sockaddr *>(&server_), sizeof(server_));
	if (bindStatus == -1) {
		throw std::runtime_error{ std::string{ "Can not bind socket: " } + std::string{ strerror(errno) } };
	}

	auto connectionStatus = listen(sockFd_, BACKLOG);
	if (connectionStatus == -1) {
		throw std::runtime_error{ "Error: could not listen the specified TCP port" };
	}
	std::cout << "Server has been started and listening port TCP/" << config_["ListenPort"] << '\n';
	printPrompt();
}

// destructor
ChatServer::~ChatServer() {
	close(sockFd_);
	if(mainPid_ == getpid()) {
		wait(nullptr);
	}
}

// login availability
bool ChatServer::isLoginAvailable(const std::string& login) const {
	return users_.find(login) == users_.end();
}

void ChatServer::signUp() {
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
	
	SHA256 sha;
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
	std::cout << "User registered successfully.\n" << std::endl;
}

bool ChatServer::isValidLogin(const std::string& login) const {
	// allowed characters, verification
	for (const char c : login) {
		if (!std::isalnum(c) && c != '-' && c != '_') {
			return false;
		}
	}
	return true;
}

void ChatServer::signIn() {
	if (loggedUser_ != nullptr) {
		std::cout << "For log in you must sign out first. Enter '/logout' to sign out\n" << std::endl;
		return;
	}

	std::string login, password, hash;
	
	auto tokens = Chat::split(message_, ":");
	login = tokens[1];
	password = tokens[2];

	SHA256 sha;
	sha.update(password);
	uint8_t *digest = sha.digest();
	hash = SHA256::toString(digest);
	delete[] digest;

	auto it = users_.find(login);
	if (it == users_.end() || 
		it->second.getLogin() != login || 
		it->second.getPassword() != hash) {
		// invalid argument passed
		clearPrompt();
		std::cout << "Login failed for user '" << login << "' from " << inet_ntoa(client_.sin_addr) << std::endl;
		printPrompt();
	}
	else {
		it->second.login();
		clearPrompt();
		std::cout << "User " << it->second.getName() << " successfully logged in" << std::endl;
		printPrompt();
	}
}

void ChatServer::signOut() {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}

	loggedUser_ = nullptr;
}

void ChatServer::removeUser(ChatUser& user) {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}
	signOut();
	users_.erase(user.getLogin());
	usersFileMustBeUpdated_ = true;
	std::cout << "User removed successfully\n" << std::endl;
}

void ChatServer::sendMessage(const std::string& message) {
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

void ChatServer::sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText) {
	if (users_.find(receiverName) == users_.end()) {
		throw std::invalid_argument("User @" + receiverName + " does not exist");
	}
	messages_.emplace_back(std::make_shared<PrivateMessage>(sender.getLogin(), receiverName, messageText));
}

void ChatServer::sendBroadcastMessage(ChatUser& sender, const std::string& message) {
	// Dynamically allocate memory for new message
	messages_.emplace_back(std::make_shared<BroadcastMessage>(sender.getLogin(), message, users_));
}

void ChatServer::work() {
	socklen_t length = sizeof(client_);
	int consolePid = fork();
	if (consolePid == 0) {
		startConsole();
	}
	else {
		// Clearing zombie processes automatically
		signal(SIGCHLD, SIG_IGN);

		while (mainLoopActive_) {
			socklen_t length = sizeof(client_);
			auto connection = accept(sockFd_, reinterpret_cast<sockaddr *>(&client_), &length);
			int clientPid = fork();
			if (clientPid == 0) {
				processNewClient(connection);
			}
		}
	}
}

void ChatServer::startConsole() const {

}

void ChatServer::processNewClient(int connection) {
			clearPrompt();
			std::cout << "Client connected from " <<
				inet_ntoa(client_.sin_addr) <<
				':' << ntohs(client_.sin_port) << std::endl;
			printPrompt();
			while (true) {
				try {
					std::fill(message_, message_ + MESSAGE_LENGTH, '\0');
					auto bytes = read(connection, message_, MESSAGE_LENGTH);
					if (bytes == -1) {
						throw std::runtime_error{
							std::string{ "Error while reading from socket: " } + 
							std::string{ strerror(errno) } 
						};
					}
					if (bytes == 0) {
						clearPrompt();
						std::cout << "Client with address " << 
							inet_ntoa(client_.sin_addr) << 
							':' << ntohs(client_.sin_port) << 
							" has been disconnected\n" << std::endl;
						break;
						printPrompt();
					}

					clearPrompt();
					std::cout << "Received " << bytes << " bytes: " << message_ << std::endl;
					printPrompt();
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
					else if (
						strncmp(message_, "/exit", 5) == 0 ||
						strncmp(message_, "/quit", 5) == 0) {
						// closing the program
						break;
					}
					else if (loggedUser_ != nullptr) {
						// if there is an authorized user, we send a message
						//sendMessage(inputText);
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

			if (usersFileMustBeUpdated_) {
				try {
					saveUsers();
				}
				catch (const std::runtime_error &e) {
					std::cerr << e.what() << std::endl;
				}
			}
			/* try {
				saveMessages();
			}
			catch (const std::runtime_error &e) {
				std::cerr << e.what() << std::endl;
			} */
}

void ChatServer::checkUnreadMessages() {
	for (int i = 0; i < messages_.size(); ++i) {
		messages_[i]->printIfUnreadByUser(loggedUser_->getLogin());
	}
}

void ChatServer::saveUsers() const {
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

void ChatServer::saveMessages() const {	
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

void ChatServer::loadUsers() {
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

void ChatServer::loadMessages() {
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

void ChatServer::printPrompt() const {
	std::cout << PROMPT << ' ';
	std::cout.flush();
}

unsigned int ChatServer::getPromptLength() const {
	// Prompt length including trailing whitespace
	return PROMPT.length() + 1;
}

void ChatServer::clearPrompt() const {
	auto length = getPromptLength();
	for (unsigned i = 0; i < length; ++i) {
		// Print backspace
		std::cout << '\b';
	}
	std::cout.flush();
}

void ChatServer::printSystemInformation() const {
#if defined(__linux__)
	utsname uts;
	uname(&uts);

	std::cout << "Current process ID: " << mainPid_ << std::endl;
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
std::string ChatServer::getLiteralOSName(OSVERSIONINFOEX &osv) const {
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
