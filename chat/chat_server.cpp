#include "chat_server.h"
#include "SHA256.h"
#include "project_lib.h"

#include <functional>
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

void ChatServer::checkLogin(int connection) const {
	auto tokens = Chat::split(std::string{ message_ }, ":");
	strcpy(message_, "/response:");
	if (tokens.size() < 2) {
		strcat(message_, "busy");
	}
	else if (!isLoginAvailable(tokens[1])) {
		strcat(message_, "busy");
	}
	else {
		strcat(message_, "available");
	}
	
	auto bytes = write(connection, message_, MESSAGE_LENGTH);
	printPrompt();
}

void ChatServer::signUp(int connection) {
	std::string hash;
	auto tokens = Chat::split(message_, ":");
	// 0: cmd, 1: login, 2: password, 3: name
	if (tokens.size() < 4 || (tokens[1].empty() || tokens[2].empty() || tokens[3].empty())) {
		throw std::invalid_argument("Login and password cannot be empty.");
		clearPrompt();
		std::cout << "Signup attemp failed from " << getClientIpAndPort() << std::endl;
		printPrompt();

		return;
	}
	
	SHA256 sha;
	sha.update(tokens[2]);
	uint8_t *digest = sha.digest();
	hash = SHA256::toString(digest);
	delete[] digest;

	users_.emplace(tokens[1], ChatUser(tokens[1], hash, tokens[3]));
	strcpy(message_, "/response:success");
	auto bytes = write(connection, message_, MESSAGE_LENGTH);
	clearPrompt();
	std::cout << "User '" << tokens[1] << "' has bin registered" << std::endl;
	printPrompt();
	try {
		users_.at(tokens[1]).save(USER_CONFIG);
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}
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

void ChatServer::signIn(int connection) {
	if (!loggedUser_.empty()) {
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
		std::cout << "Login failed for user '" << login << "' from " << getClientIpAndPort() << std::endl;
		std::string answer{ "/response:fail" };
		std::cout << "Sending response: " << answer.c_str() << std::endl;
		auto bytes = write(connection, answer.c_str(), answer.length() + 1);
		printPrompt();
	}
	else {
		it->second.login();
		clearPrompt();
		std::cout << "User '" << it->second.getLogin() << "' successfully logged in" << std::endl;
		std::string answer{ std::string{ "/response:success:" } + it->second.getName() };
		auto bytes = write(connection, answer.c_str(), answer.length() + 1);
		loggedUser_ = login;
		printPrompt();
	}
}

void ChatServer::signOut() {
	clearPrompt();
	std::cout << "User '" << loggedUser_ << "' logged out at " << getClientIpAndPort() << std::endl;
	printPrompt();
	users_.at(loggedUser_).logout();
	loggedUser_.clear();
}

void ChatServer::removeUser(int connection) {
	if (!users_.at(loggedUser_).isLoggedIn()) {
		strcpy(message_, "/response:fail");
		write(connection, message_, MESSAGE_LENGTH);
		return;
	}
		
	strcpy(message_, "/response:success");
	write(connection, message_, MESSAGE_LENGTH);
	users_.erase(loggedUser_);
	clearPrompt();
	std::cout << "User '" << loggedUser_ << "' has been removed" << std::endl;
	printPrompt();
	signOut();
	usersFileMustBeUpdated_ = true;
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
			sendPrivateMessage(users_.at(loggedUser_), receiverName, messageText);
		}
	}
	else {
		sendBroadcastMessage(users_.at(loggedUser_), message);
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
	consolePid_ = fork();
	if (consolePid_ == 0) {
		startConsole();
	}
	else {
		while (mainLoopActive_) {
			socklen_t length = sizeof(client_);
			auto connection = accept(sockFd_, reinterpret_cast<sockaddr *>(&client_), &length);
			int clientPid = fork();
			if (clientPid == 0) {
				processNewClient(connection);
			}
			else {
				children_.insert(clientPid);
			}
		}
	}
}

void ChatServer::startConsole() const {
	std::string cmd;
	while(mainLoopActive_) {
		getline(std::cin, cmd);
		if (cmd == "/quit" || cmd == "/exit") {
			break;
		}
		if (mainLoopActive_) {
			printPrompt();
		}
	}
}

void ChatServer::writeBuffer(const std::string &text) const {
	while (fs::exists(BUFFER_LOCK)) {
		sleep(1);
	}
	std::ofstream{BUFFER_LOCK, std::ios::out | std::ios::trunc};
	std::ofstream buffer{ BUFFER, std::ios::out | std::ios::trunc };
	buffer << text;
	buffer.close();
	fs::remove(BUFFER_LOCK);
}

void ChatServer::cleanExit() {
	if (mainPid_ != getpid()) {
		terminateChild();
	}
	mainLoopActive_ = false;
	clearPrompt();
	std::cout << "\nPreparing server for clean exit..." << std::endl;
	std::cout << "Killing all childs..." << std::endl;
	for (auto child: children_) {
		kill(child, SIGTERM);
	}
	std::cout << "Closing socket..." << std::endl;
	close(sockFd_);
	std::cout << "Exiting from main process..." << std::endl;
	clearPrompt();
	
	exit(EXIT_SUCCESS);	
}

void ChatServer::terminateChild() const {
	if (consolePid_ == getpid()) {
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
	clearPrompt();
	std::cout << "Exiting from child " << getpid() << "..." << std::endl;
	printPrompt();
	exit(EXIT_SUCCESS);
}

void ChatServer::childDeathHandler(int signum) {
	pid_t pid;
	while ((pid = waitpid(-1, nullptr, WNOHANG)) > 0) {
		if (pid == consolePid_) {
			cleanExit();
		}
		else {
			auto it = children_.find(pid);
			if (it != children_.end()) {
				children_.erase(it);
			}
		}
	}
}

void ChatServer::sigIntHandler(int signum) {
	if (mainPid_ != getpid()) {
		terminateChild();
	}
	else {
		std::cout << "\nCaught interrupt signal!" << std::endl;
		kill(consolePid_, SIGTERM);
		cleanExit();
	}
}

void ChatServer::sigTermHandler(int signum) {
	if (mainPid_ != getpid()) {
		terminateChild();
	}
	else {
		std::cout << "\nCaught terminate signal!" << std::endl;
		kill(consolePid_, SIGTERM);
		cleanExit();
	}
}

void ChatServer::processNewClient(int connection) {
	clearPrompt();
	std::cout << "Client connected from " << getClientIpAndPort() << std::endl;
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
				std::cout << "Client with address " << getClientIpAndPort() << " has been disconnected\n" << std::endl;
				printPrompt();
				break;
			}

			clearPrompt();
			std::cout << "Received " << bytes << " bytes: " << message_ << std::endl;
			printPrompt();
			if (strncmp(message_, "/help", 5) == 0) {
				// output help
				Chat::displayHelp();
			}
			else if (strncmp(message_, "/checklogin", 11) == 0) {
				checkLogin(connection);
			}
			else if (strncmp(message_, "/signup", 7) == 0) {
				// registration
				signUp(connection);
			}
			else if (strncmp(message_, "/signin", 7) == 0) {
				// authorization
				signIn(connection);
			}
			else if (strncmp(message_, "/logout", 7) == 0) {
				// logout
				signOut();
			}
			else if (strncmp(message_, "/remove", 7) == 0) {
				// removing current user
				if (!loggedUser_.empty()) {
					removeUser(connection);
				}
			}
			else if (
				strncmp(message_, "/exit", 5) == 0 ||
				strncmp(message_, "/quit", 5) == 0) {
				// closing the program
				break;
			}
			else if (!loggedUser_.empty()) {
				// if there is an authorized user, we send a message
				//sendMessage(inputText);
			}
			else {
				std::cout << 
					"the command is not recognized, \n"
					"to output help, type /help\n" 
				<< std::endl;
			}
			if (!loggedUser_.empty()) {
				checkUnreadMessages();
			}
		}
		catch (std::invalid_argument e) {
			// exception handling
			std::cout << "Error: " << e.what() << "\n" << std::endl;
		}
	}
	cleanExit();
}

void ChatServer::checkUnreadMessages() {
	for (int i = 0; i < messages_.size(); ++i) {
		messages_[i]->printIfUnreadByUser(users_.at(loggedUser_).getLogin());
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
	// Using ANSI escape sequence CSI2K and then carriage return
	std::cout << "\x1B[2K\r";
	std::cout.flush();
}

std::string ChatServer::getClientIpAndPort() const {
	std::string result;
	return (result + inet_ntoa(client_.sin_addr) + ":" + std::to_string(ntohs(client_.sin_port)));
}

void ChatServer::updateUserList() const {
	const std::string USERLIST_TEMP{  "/tmp/chat_server_userlist.tmp" };
	while (fs::exists(USERLIST_LOCK)) {
		sleep(1);
	}
	std::ofstream{ USERLIST_LOCK, std::ios::out | std::ios::trunc };

	std::set<std::string> usersInFile;
	fs::copy(USER_CONFIG, USERLIST_TEMP);
	std::ifstream is{ USERLIST_TEMP, std::ios::in };
	std::ofstream os{ USER_CONFIG, std::ios::out | std::ios::trunc };
	while (!is.eof()) {
		std::string user, password, name, temp;
		getline(is, user);
		if (user.empty()) {
			break;
		}
		getline(is, password);
		getline(is, name);
		if (users_.find(user) == users_.end()) {
			// User has been removed - don't write his information to file
			continue;
		}
		os << user << '\n' << password << '\n' << name << std::endl;
	}
	os.close();
	is.close();
	fs::remove(USERLIST_TEMP);
	fs::remove(USERLIST_LOCK);
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
