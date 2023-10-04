#include "chat_mgr.h"

#include <string>
#include <fstream>
#include <filesystem>
#include <stdexcept>
#if defined(__linux__)
#include <unistd.h>
#include <sys/utsname.h>
#elif defined(_WIN64) or defined(_WIN32)
#pragma comment(lib, "ntdll")

extern "C" NTSTATUS __stdcall RtlGetVersion(OSVERSIONINFOEXW * lpVersionInformation);
#endif

namespace fs = std::filesystem;

// construct
ChatMgr::ChatMgr() {
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
		"Welcome to the chat,\n"
		"to view help, type /help\n"
		"Registered users: ";
	for (const auto kv: users_) {
		std::cout << '\'' << kv.first << "' ";
	}
	std::cout << '\n' << std::endl;
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

	users_.emplace(login, ChatUser(login, password, name));
	try {
		users_.at(login).save(USER_CONFIG);
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}
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
	usersFileMustBeUpdated_ = true;
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
				if (loggedUser_) {
					removeUser(*loggedUser_);
				}
			}
			else if (
				input_text == "/exit" ||
				input_text == "/quit") {
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

	if (usersFileMustBeUpdated_) {
		try {
			saveUsers();
		}
		catch (const std::runtime_error &e) {
			std::cerr << e.what() << std::endl;
		}
	}
	try {
		saveMessages();
	}
	catch (const std::runtime_error &e) {
		std::cerr << e.what() << std::endl;
	}
}

void ChatMgr::checkUnreadMessages() {
	for (int i = 0; i < messages_.size(); ++i) {
		messages_[i]->printIfUnreadByUser(loggedUser_->getLogin());
	}
}

void ChatMgr::saveUsers() const {
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

void ChatMgr::saveMessages() const {	
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

void ChatMgr::loadUsers() {
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

void ChatMgr::loadMessages() {
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

void ChatMgr::printSystemInformation() const {
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
std::string ChatMgr::getLiteralOSName(OSVERSIONINFOEX &osv) const {
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
