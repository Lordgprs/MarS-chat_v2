#include "chat_server.h"
#include "SHA256.h"
#include "project_lib.h"

#include <functional>
#include <string>
#include <fstream>
#include <filesystem>
#if defined(__linux__)
extern "C" {
	#include <sys/utsname.h>
	#include <errno.h>
	#include <sys/select.h>
	#include <cstdlib>
}
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
		setUsersInactive();
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}

	// We don't want to load message history at startup
	// try {
	//	loadMessages(HISTORY_LOG);
	// }
	// catch (const std::runtime_error &e) {
	// 	std::cerr << e.what() << std::endl;
	// }

	
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
	std::cout <<
		"Welcome to the chat admin console. "
		"This chat server supports multiple client login and creates own process for each one.\n"
		"Type /help to view help" << std::endl;

	if(!users_.empty()) {
		std::cout << "Registered users: ";
		for (const auto &it: users_) {
			std::cout << std::quoted(it.first) << ' ';
		}
	}

	std::cout << "\n\nServer has been started and listening port TCP/" << config_["ListenPort"] << std::endl;
	printPrompt();
}

void ChatServer::setUsersInactive() const {
	Mysql mysql;
	mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
	mysql.query("DELETE FROM active_sessions");
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
	try {
		Mysql mysql;
		try {
			mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
			mysql.query("SELECT COALESCE (MAX(`id`), -1) FROM `users`");
			auto rows = mysql.fetchAll();
			int new_id{ std::stoi(rows.front().at(0)) };
			++new_id;
	
			SHA256 sha;
			sha.update(tokens[2]);
			uint8_t *digest = sha.digest();
			hash = SHA256::toString(digest);
			delete[] digest;

			users_.emplace(tokens[1], ChatUser(new_id, tokens[1], hash, tokens[3]));
			strcpy(message_, "/response:success");
			auto bytes = write(connection_, message_, MESSAGE_LENGTH);
			clearPrompt();
			std::cout << "User '" << tokens[1] << "' has bin registered" << std::endl;
			printPrompt();
			users_.at(tokens[1]).save(mysql);
		}
		catch (const std::runtime_error &e) {
			clearPrompt();
			std::cout << "Error: can not save user information to database (" << e.what() << ")" << std::endl;
			printPrompt();
		}
		//while(fs::exists(USERLIST_LOCK)) {
		//	sleep(1);
		//}
		//std::ofstream lock{ USERLIST_LOCK, std::ios::out | std::ios::trunc };
		//if (!lock.is_open()) {
		//	throw std::runtime_error{ "Error: can not create userlist lock file!" };
		//}
		//lock.close();
		//try {
		//	users_.at(tokens[1]).save(USER_CONFIG);
		//}
		//catch (const std::out_of_range &e) {
		//	clearPrompt();
		//	std::cout << "Error: can not save user information to file (" << e.what() << std::endl;
		//	printPrompt();
		//}
		//fs::remove(USERLIST_LOCK);
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
		auto bytes = write(connection_, message_, MESSAGE_LENGTH);
	}
	else {
		updateActiveUsers();
		if (users_.at(login).isLoggedIn()) {
			strcpy(message_, "/response:loggedin");
			clearPrompt();
			std::cout << "User " << std::quoted(login) << " is already logged in" << std::endl;
			std::cout << "Sending response: " << message_ << std::endl;
			auto bytes = write(connection_, message_, MESSAGE_LENGTH);
			printPrompt();
			return;
		}

		it->second.login();
		clearPrompt();
		std::cout << "User " << std::quoted(it->second.getLogin()) << " successfully logged in" << std::endl;
		strcpy(message_, "/response:success:");
		strcat(message_, it->second.getName().c_str());
		strcat(message_, ":");
		strcat(message_, std::to_string(it->second.getUserId()).c_str());
		auto bytes = write(connection_, message_, MESSAGE_LENGTH);
		loggedUser_ = login;
		writeSessionInfo();
	}
	printPrompt();
}

void ChatServer::writeSessionInfo() const {
	if (loggedUser_.empty()) {
		clearPrompt();
		std::cout << "Error: trying to save user information without login" << std::endl;
		printPrompt();
		return;
	}

	try {
		Mysql mysql;
		try {
			std::stringstream ss;

			mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
			ss << "DELETE FROM `active_sessions` WHERE `user_id` = " << users_.at(loggedUser_).getUserId();
			mysql.query(ss.str());
			ss.str(std::string{});
			ss << 
				"INSERT INTO `active_sessions` (`user_id`, `ip`, `port`) VALUES (" <<
				users_.at(loggedUser_).getUserId() <<
				", INET_ATON('" << getClientIp() << "'), "
				<< getClientPort() << ")";
			mysql.query(ss.str());
		}
		catch (const std::runtime_error &e) {
			clearPrompt();
			std::cout << "Error: can not save user information to database (" << e.what() << ")" << std::endl;
			printPrompt();
		}
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cerr << "Error: cannot connect to database (" << e.what() << ")" << std::endl;
		printPrompt();
	}
}

void ChatServer::removeSessionInfo(const std::string &username) const {
	if (username.empty()) {
		throw std::runtime_error { "Failed to remove user file without correct username" };
	}

	try {
		Mysql mysql;
		try {
			std::stringstream ss;

			mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
			ss << "DELETE FROM `active_sessions` WHERE `user_id` = " << users_.at(username).getUserId();
			mysql.query(ss.str());
		}
		catch (const std::runtime_error &e) {
			clearPrompt();
			std::cout << "Error: can not remove user session from database (" << e.what() << ")" << std::endl;
			printPrompt();
		}
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cerr << "Error: cannot connect to database (" << e.what() << ")" << std::endl;
		printPrompt();
	}
}

void ChatServer::signOut() {
	clearPrompt();
	std::cout << "User '" << loggedUser_ << "' logged out at " << getClientIpAndPort() << std::endl;
	printPrompt();
	try {
		users_.at(loggedUser_).logout();
	}
	catch (const std::out_of_range &e) {
		clearPrompt();
		std::cout << "Error: can not log out (" << e.what() << std::endl;
		printPrompt();
	}
	removeSessionInfo(loggedUser_);
	loggedUser_.clear();
}

void ChatServer::removeUser(const std::string &cmd) {
	std::string removingUser{ cmd.substr(7, cmd.length() - 7) };
	std::erase(removingUser, ' ');
	loadUsers();
	updateActiveUsers();
	auto it = users_.find(removingUser);
	if (it == users_.end()) {
		clearPrompt();
		std::cout << "User " << std::quoted(removingUser) << " does not exist" << std::endl;
		return;
	}
	if (it->second.isLoggedIn()) {
		clearPrompt();
		std::cout << "Can not remove user " << std::quoted(removingUser) << " because he/she is logged in now. Kick him/her first" << std::endl;
		return;
	}
	
	clearPrompt();
	std::cout << "Removing user " << std::quoted(removingUser) << std::endl;
	removeUserFromFile(removingUser);	
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
	std::cout << "User " << std::quoted(removingUser) << " has been removed" << std::endl;
	printPrompt();
}

void ChatServer::sendMessage() {
	std::string message{ message_ };
	if (message.empty()) {
		// message can no be empty
		return;
	}

	if (message[0] == '@') {
		size_t pos = message.find(' ');
		if (pos != std::string::npos) {
			std::string receiverName = message.substr(1, pos - 1);
			clearPrompt();
			printPrompt();
			std::string messageText = message.substr(pos + 1);
			try {
				sendPrivateMessage(users_.at(loggedUser_), receiverName, messageText);
			}
			catch (const std::out_of_range &e) {
				clearPrompt();
				std::cout << "Error: can not send private message (" << e.what() << std::endl;
				printPrompt();
			}
		}
	}
	else {
		try {
			sendBroadcastMessage(users_.at(loggedUser_), message);
		}
		catch (const std::out_of_range &e) {
			clearPrompt();
			std::cout << "Error: can not send broadcast message (" << e.what() << std::endl;
			printPrompt();
		}
	}
}

void ChatServer::writeMessageToHistory(const std::shared_ptr<ChatMessage> message) const {
	try {
		Mysql mysql;
		try {
			mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
			message->save(mysql);
		}
		catch (const std::runtime_error &e) {
			clearPrompt();
			std::cout << "Error: can not save message to database (" << e.what() << ")" << std::endl;
			printPrompt();
		}
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cout << "Error: can not connect to database (" << e.what() << ")" << std::endl;
		printPrompt();
	}
/*	while (fs::exists(HISTORY_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ HISTORY_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open history lock file for writing" };
	}
	try {
		message->save(HISTORY_LOG);
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cout << "Error: can not write message to history log (" << e.what() << ")" << std::endl;
		printPrompt();
	}
	lock.close();
	fs::remove(HISTORY_LOCK);	*/
}

void ChatServer::sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText) {
	if (users_.find(receiverName) == users_.end()) {
		throw std::invalid_argument("RECEIVER_DOES_NOT_EXIST");
	}

	auto newMessage = std::make_shared<PrivateMessage>(sender.getLogin(), receiverName, messageText);
	writeMessageToHistory(newMessage);
	
	while (fs::exists(MESSAGE_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ MESSAGE_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open messages lock file for writing" };
	}
	try {
		std::string userMessages{ TEMP_DIR + "/messages-" + receiverName + ".txt" }; 
		newMessage->save(userMessages);
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cout << "Error: can not write message to temporary file for futher transfer (" << e.what() << ")" << std::endl;
		printPrompt();
	}
	lock.close();
	fs::remove(MESSAGE_LOCK);	
}

void ChatServer::sendBroadcastMessage(ChatUser& sender, const std::string& message) {
	// Dynamically allocate memory for new message
	auto newMessage = std::make_shared<BroadcastMessage>(sender.getLogin(), message, users_);
	
	writeMessageToHistory(newMessage);
	
	while (fs::exists(MESSAGE_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ MESSAGE_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open messages lock file for writing" };
	}
	lock.close();
	try {
		for(const auto &it: users_) {
			if (it.second.getLogin() == loggedUser_) {
				continue;
			}
			std::string userMessages{ TEMP_DIR + "/messages-" + it.second.getLogin() + ".txt" }; 
			newMessage->save(userMessages);
		}
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cout << "Error: can not write message to temporary file for futher transfer (" << e.what() << ")" << std::endl;
		printPrompt();
	}
	fs::remove(MESSAGE_LOCK);
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
			removeSessionInfo(tokens[1]);
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
		int clientPid;
		while (mainLoopActive_) {
			socklen_t length = sizeof(client_);
			try {
				connection_ = accept(sockFd_, reinterpret_cast<sockaddr *>(&client_), &length);
			}
			catch (const std::out_of_range &e) {
				clearPrompt();
				std::cout << "Error: out of range while trying to call accept() (" << e.what() << ')' << std::endl;
				printPrompt();
			}
			clientPid = fork();
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
		else if (cmd.substr(0, 7) == "/remove") {
			removeUser(cmd);
		}
		printPrompt();
	}
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
					removeSessionInfo(user["Username"]);
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
			if (!loggedUser_.empty()) {
				checkUnreadMessages();
			}

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
				sendMessage();
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
	while (fs::exists(MESSAGE_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ MESSAGE_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not open messages lock file for writing" };
	}
	lock.close();
	std::string userMessages{ TEMP_DIR + "/messages-" + loggedUser_ + ".txt" }; 
	loadMessages(userMessages);
	fs::remove(userMessages);

	for (const auto &message: messages_) {
		strcpy(message_, message->createTransferString().c_str());
		write(connection_, message_, MESSAGE_LENGTH);
	}
	fs::remove(MESSAGE_LOCK);
}

void ChatServer::saveMessages() const {	
	std::cout << "Saving chat history to file " << HISTORY_LOG << "..." << std::endl;
	std::ofstream file(HISTORY_LOG, std::ios::out | std::ios::trunc);
	if (!file.is_open()) {
		throw std::runtime_error{ "Cannot open file " + HISTORY_LOG + " for write" };
	}
	file.close();

	for (const auto &msg: messages_) {
		msg->save(HISTORY_LOG);		
	}
}

void ChatServer::loadUsers() {
	Mysql mysql;
	mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
	if (!mysql.query("SELECT `id`, `login`, `name`, `password_hash` FROM `users` ORDER BY `id`")) {
		std::stringstream ss;
		ss << "MySQL error: " << mysql.getError() << std::endl;
		throw std::runtime_error{ ss.str() };
	}
	auto users_list = mysql.fetchAll();
	users_.clear();
	for (auto &user_data: users_list) {
		users_.emplace(user_data[1], ChatUser(std::stoi(user_data[0]), user_data[1], user_data[2], user_data[3]));
	}


	/*while (fs::exists(USERLIST_LOCK)) {
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
	fs::remove(USERLIST_LOCK);*/
}

void ChatServer::saveUsers() const {
	try {
		Mysql mysql;
		try {
			mysql.open(config_["DBName"], config_["DBHost"], config_["DBUser"], config_["DBPassword"]);
			for (auto it = users_.begin(); it != users_.end(); ++it) {
				it->second.save(mysql);		
			}
		}
		catch (const std::runtime_error &e) {
			clearPrompt();
			std::cout << "Error: can not save user information to database (" << e.what() << ")" << std::endl;
			printPrompt();
		}
	}
	catch (const std::runtime_error &e) {
		clearPrompt();
		std::cout << "Error: can not connect to database (" << e.what() << ")" << std::endl;
		printPrompt();
	}
}

void ChatServer::loadMessages(const std::string &filename) {
	if (!messages_.empty()) {
		for(auto &message: messages_) {
			message.reset();
		}
		messages_.clear();
	}
	if(!fs::exists(filename)) {
		return;
	}

	std::ifstream file(filename, std::ios::in);
	if (!file.is_open()) {
		throw std::runtime_error{ "Error: cannot open file " + filename + " for read" };
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
	return getClientIp() + ":" + getClientPort();
}

std::string ChatServer::getClientIp() const {
	return std::string{ inet_ntoa(client_.sin_addr) };
}

std::string ChatServer::getClientPort() const {
	return std::to_string(ntohs(client_.sin_port));
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
