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
#include <sys/select.h>
#elif defined(_WIN64) or defined(_WIN32)
#pragma comment(lib, "ntdll")

extern "C" NTSTATUS __stdcall RtlGetVersion(OSVERSIONINFOEXW * lpVersionInformation);
#endif

namespace fs = std::filesystem;

// constructor
ChatServer::ChatServer() {
	mainPid_ = getpid();
	printSystemInformation();

	if (fs::exists(TEMP_DIR)) {
		throw std::runtime_error{
			std::string{ "Temporary directory " } +
			TEMP_DIR +
			" exists. Maybe server is already running? "
			"If server is not running, please delete temporary directory before start"
		};
	}
	fs::create_directory(TEMP_DIR);
	fs::create_directory(USERS_DIR);
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

void ChatServer::displayHelp() const {
	std::cout << "Available commands:\n"
		" /help: chat help, displays a list of commands to manage the chat\n"
		" /list: list connected users\n"
		" /kick <username>: kick connected user\n"
		" /remove: delete inactive user\n"
		" /exit, /quit, Ctrl-C: close the program\n"
		<< std::endl;
}

// login availability
bool ChatServer::isLoginAvailable(const std::string& login) const {
	return users_.find(login) == users_.end();
}

void ChatServer::checkLogin() const {
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
	
	auto bytes = write(connection_, message_, MESSAGE_LENGTH);
	printPrompt();
}

void ChatServer::signUp() {
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
	auto bytes = write(connection_, message_, MESSAGE_LENGTH);
	clearPrompt();
	std::cout << "User '" << tokens[1] << "' has bin registered" << std::endl;
	printPrompt();
	try {
		while(fs::exists(USERLIST_LOCK)) {
			sleep(1);
		}
		std::ofstream lock{ USERLIST_LOCK, std::ios::out | std::ios::trunc };
		if (!lock.is_open()) {
			throw std::runtime_error{ "Error: can not create userlist lock file!" };
		}
		lock.close();
		users_.at(tokens[1]).save(USER_CONFIG);
		fs::remove(USERLIST_LOCK);
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

void ChatServer::signIn() {
	if (!loggedUser_.empty()) {
		std::cout << "For log in you must sign out first. Enter '/logout' to sign out\n" << std::endl;
		return;
	}

	std::string login, password, hash;
	
	auto tokens = Chat::split(message_, ":");
	login = tokens[1];
	password = tokens[2];

	updateActiveUsers();
	if (users_.at(login).isLoggedIn()) {
		strcpy(message_, "/response:loggedin");
		clearPrompt();
		std::cout << "User " << std::quoted(login) << " is already logged in" << std::endl;
		std::cout << "Sending response: " << message_ << std::endl;
		auto bytes = write(connection_, message_, MESSAGE_LENGTH);
		printPrompt();
	}

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
		std::cout << "Login failed for user " << std::quoted(login) << " from " << getClientIpAndPort() << std::endl;
		strcpy(message_, "/response:fail");
		std::cout << "Sending response: " << message_ << std::endl;
		auto bytes = write(connection_, message_, MESSAGE_LENGTH);
		printPrompt();
	}
	else {
		it->second.login();
		clearPrompt();
		std::cout << "User " << std::quoted(it->second.getLogin()) << " successfully logged in" << std::endl;
		strcpy(message_, "/response:success:");
		strcat(message_, it->second.getName().c_str());
		auto bytes = write(connection_, message_, MESSAGE_LENGTH);
		loggedUser_ = login;
		writeUserFile();
		printPrompt();
	}
}

void ChatServer::writeUserFile() const {
	if (loggedUser_.empty()) {
		clearPrompt();
		std::cout << "Error: trying to save user information without login" << std::endl;
		printPrompt();
		return;
	}

	const std::string USER_LOCK { USERS_DIR + "/" + loggedUser_ + ".lock" };
	const std::string USER_INFO { USERS_DIR + "/" + loggedUser_ + ".info" };
	while(fs::exists(USER_LOCK)) {
		sleep(1);
	}

	std::ofstream lock{ USER_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open user lock file for writing" };
	}
	lock.close();
	std::ofstream userInfo{ USER_INFO };
	if (!userInfo.is_open()) {
		fs::remove(USER_LOCK);
		throw std::runtime_error{ "Error: can not open user information file for writing" };
	}
	userInfo <<
		"Username = " << loggedUser_ << '\n' <<
		"Pid = " << getpid() << '\n' <<
		"Address = " << getClientIpAndPort() << std::endl; 
	userInfo.close();

	fs::remove(USER_LOCK);
}

void ChatServer::removeUserFile(const std::string &username) const {
	if (username.empty()) {
		throw std::runtime_error { "Failed to remove user file without correct username" };
	}

	const std::string USER_LOCK { USERS_DIR + "/" + username + ".lock" };
	const std::string USER_INFO { USERS_DIR + "/" + username + ".info" };
	while(fs::exists(USER_LOCK)) {
		sleep(1);
	}

	std::ofstream lock{ USER_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open user lock file for writing" };
	}
	lock.close();
	if (fs::exists(USER_INFO)) {
		fs::remove(USER_INFO);
	}
	fs::remove(USER_LOCK);
}

void ChatServer::signOut() {
	clearPrompt();
	std::cout << "User '" << loggedUser_ << "' logged out at " << getClientIpAndPort() << std::endl;
	printPrompt();
	users_.at(loggedUser_).logout();
	removeUserFile(loggedUser_);
	loggedUser_.clear();
}

void ChatServer::removeUser() {
	std::string removingUser{ loggedUser_ };
	if (!users_.at(removingUser).isLoggedIn()) {
		strcpy(message_, "/response:fail");
		write(connection_, message_, MESSAGE_LENGTH);
		return;
	}
		
	strcpy(message_, "/response:success-test");
	write(connection_, message_, MESSAGE_LENGTH);
	signOut();
	removeUserFromFile(removingUser);
	loggedUser_.clear();
	clearPrompt();
	std::cout << "User '" << removingUser << "' has been removed" << std::endl;
	printPrompt();
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

void ChatServer::listActiveUsers() {
	updateActiveUsers();
	clearPrompt();
	std::cout << "Active users:" << std::endl;
	for (const auto &user: activeUsers_) {
		std::cout << "Name: " << user["Username"] << "; Address: " << user["Address"] << "; Pid: " << user["Pid"] << std::endl;
	}
	std::cout << std::endl;
}

void ChatServer::kickClient(const std::string &cmd) {
	auto tokens = Chat::split(cmd, " ");
	if (tokens.size() != 2) {
		throw std::invalid_argument{ "Error: invalid format" };
	}
	updateActiveUsers();
	auto it = users_.find(tokens[1]);
	if (it == users_.end()) {
		throw std::invalid_argument{ "Error: user not exist" };
	}
	if (!it->second.isLoggedIn()) {
		throw std::invalid_argument{ "Error: user is not logged in" };
	}
	for (const auto &user: activeUsers_) {
		if (user["Username"] == tokens[1]) {
			kill(stoi(user["Pid"]), SIGTERM);
			users_.at(tokens[1]).logout();
			removeUserFile(tokens[1]);
		}
	}
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
			connection_ = accept(sockFd_, reinterpret_cast<sockaddr *>(&client_), &length);
			int clientPid = fork();
			if (clientPid == 0) {
				processNewClient();
			}
			else {
				children_.insert(clientPid);
			}
		}
	}
}

void ChatServer::startConsole() {
	std::string cmd;
	while(mainLoopActive_) {
		getline(std::cin, cmd);
		if (cmd == "/quit" || cmd == "/exit") {
			break;
		}
		if (cmd == "/help") {
			displayHelp();
		}
		else if (cmd.substr(0, 5) == "/kick") {
			kickClient(cmd);
		}
		else if (cmd == "/list") {
			listActiveUsers();
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

void ChatServer::updateActiveUsers() {
	activeUsers_.clear();
	for (const auto &file: fs::directory_iterator(USERS_DIR)) {
		activeUsers_.emplace_back(file.path());
	}
	for (auto &it: users_) {
		it.second.logout();
	}
	for (const auto &user: activeUsers_) {
		auto it = users_.find(user["Username"]);
		if (it != users_.end()) {
			if(!it->second.isLoggedIn()) {
				it->second.login();
			}
		}
	}
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
	sleep(2); // Waiting 2 seconds for all children will die
	clearPrompt();
	std::cout << "Closing socket..." << std::endl;
	close(sockFd_);
	std::cout << "Removing temporary directory..." << std::endl;
	fs::remove_all(TEMP_DIR);
	std::cout << "Exiting from main process..." << std::endl;
	
	exit(EXIT_SUCCESS);	
}

void ChatServer::terminateChild() const {
	clearPrompt();
	std::cout << "Exiting from child " << getpid() << "..." << std::endl;
	if (connection_ != 0) {
		strcpy(message_, "/response:kick");
		if (write(connection_, message_, MESSAGE_LENGTH) == -1) {
			std::cerr << "Error while calling write: " << strerror(errno) << std::endl;
		}
		close(connection_);
	}
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
			updateActiveUsers();
			for (const auto &user: activeUsers_) {
				if (stoi(user["Pid"]) == pid) {
					removeUserFile(user["Username"]);
				}
			}
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

void ChatServer::processNewClient() {
	clearPrompt();
	std::cout << "Client connected from " << getClientIpAndPort() << std::endl;
	printPrompt();

	fd_set rfds;
	while (true) {
		try {
			int bytes;
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = 500000; // half second
			FD_ZERO(&rfds);
			FD_SET(connection_, &rfds);
			auto retval = select(connection_ + 1, &rfds, nullptr, nullptr, &tv);
			if (retval == -1) {
				clearPrompt();
				std::cout << "An error occured while trying to call select(): " << strerror(errno) << std::endl;
				printPrompt();
				continue;
			}
			if (retval == 0) { // select() timed out
				continue;
			}

			std::fill(message_, message_ + MESSAGE_LENGTH, '\0');
			bytes = read(connection_, message_, MESSAGE_LENGTH);
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
			if (strncmp(message_, "/checklogin", 11) == 0) {
				checkLogin();
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
				if (!loggedUser_.empty()) {
					removeUser();
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
	while (fs::exists(USERLIST_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ USERLIST_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open create userlist lock file!" };
	}
	lock.close();
	std::ifstream file(USER_CONFIG, std::ios::in);
	if (!file.is_open()) {
		fs::remove(USERLIST_LOCK);
		throw std::runtime_error{ "Error: cannot open file " + USER_CONFIG + " for read" };
	}

	users_.clear();
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
	fs::remove(USERLIST_LOCK);
}

void ChatServer::saveUsers() const {
	while (fs::exists(USERLIST_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ USERLIST_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open create userlist lock file!" };
	}
	lock.close();

	clearPrompt();
	std::cout << "Saving users information to file " << USER_CONFIG << "..." << std::endl;
	printPrompt();
	std::ofstream file(USER_CONFIG, std::ios::out | std::ios::trunc);
	if (!file.is_open()) {
		fs::remove(USERLIST_LOCK);
		throw std::runtime_error{ "Cannot open file " + USER_CONFIG + " for write" };
	}
	file.close();

	for (auto it = users_.begin(); it != users_.end(); ++it) {
		it->second.save(USER_CONFIG);		
	}
	
	fs::remove(USERLIST_LOCK);
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

void ChatServer::removeUserFromFile(const std::string &removedUser) {
	while (fs::exists(USERLIST_LOCK)) {
		sleep(1);
	}
	loadUsers();

	std::ofstream lock{ USERLIST_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open create userlist lock file!" };
	}
	lock.close();

	auto it = users_.find(removedUser);
	if (it != users_.end()) {
		users_.erase(it);
	}
	
	fs::remove(USERLIST_LOCK);
	saveUsers();
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
