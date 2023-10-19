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
	mainPid_ = getpid();
	printSystemInformation();
	fs::create_directory(TEMP_DIR);

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
	fs::remove_all(TEMP_DIR);
}

void ChatClient::cleanExit() const {
	if(mainPid_ != getpid()) {
		return;
	}

	if(pollerPid_ > 0) {
		kill(pollerPid_, SIGTERM);
	}
	close(sockFd_);
	fs::remove_all(TEMP_DIR);

	exit(EXIT_SUCCESS);
}

// login availability
bool ChatClient::isLoginAvailable(const std::string& login) const {
	strcpy(message_, "/checklogin:");
	strcat(message_, login.c_str());
	sendRequest();
	receiveResponse();
	if (strcmp(message_, "/response:busy") == 0) {
		return false;
	}

	return true;
}

void ChatClient::signUp() {
	if (loggedUser_ != nullptr) {
		std::cout << "To register a new account, enter /logout first.\n" << std::endl;
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
		throw std::invalid_argument("You can not use this login for yout registration");
	}

	if (login.empty() || password.empty()) {
		// invalid argument passed
		throw std::invalid_argument("Login and password cannot be empty.");
	}

	if (!isValidLogin(login)) {
		// invalid argument passed
		throw std::invalid_argument("Login contains invalid characters.");
	}
	
	strcpy(message_, "/signup:");
	strcat(message_, login.c_str());
	strcat(message_, ":");
	strcat(message_, password.c_str());
	strcat(message_, ":");
	strcat(message_, name.c_str());
	sendRequest();
	receiveResponse();
	if (strncmp(message_, "/response:success", 17) == 0) {
		std::cout << "User '" << login << "' registered successfully" << std::endl;
	}
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
	std::fill(message_, message_ + MESSAGE_LENGTH, '\0');
	strcpy(message_, cmd.c_str());
	sendRequest();
	std::cout << "Receiving response: " << std::endl;
	receiveResponse();
	if (strncmp(message_, "/response:success", 17) == 0) {
		std::cout << "Login successful" << std::endl;
		auto tokens = Chat::split(std::string{ message_ }, ":");
		std::string name;
		if (tokens.size() >= 3) {
			name = tokens[2];
		}
		loggedUser_ = make_shared<ChatUser>(login, password, name);
		startPoller();
	}
	else {
		std::cout << "Login failed" << std::endl;
	}
}

void ChatClient::startPoller() {
	pollerPid_ = fork();
	if (pollerPid_ < 0) {
		throw std::invalid_argument{ "Fatal error: fork() failed" };
	}
	if (pollerPid_ > 0) {
		signal(SIGCHLD, SIG_IGN);
		return;
	}

	while (true) {
		receiveResponse();
		if (strncmp(message_, "/response", 9) == 0) {
			writeResponseToFile();
			continue;
		}
		auto tokens = Chat::split(message_, "\n");
		if (tokens.size() != 3) {
			continue; // Wrong message
		}
		clearPrompt();
		std::cout << tokens[1] << ": ";
		if (tokens[0] == "PRIVATE") {
			std::cout << '@' << loggedUser_->getLogin() << ' ';
		}
		std::cout << tokens[2] << std::endl;
		printPrompt();
	}
}

void ChatClient::writeResponseToFile() const {
	while(fs::exists(RESPONSE_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ RESPONSE_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not create response lock file" };
	}
	lock.close();

	std::ofstream response{ RESPONSE, std::ios::out | std::ios::trunc };
	if (!response.is_open()) {
		throw std::runtime_error{ "Error: can not create response file" };
	}
	response << message_ << std::endl;
	response.close();

	fs::remove(RESPONSE_LOCK);
}

bool ChatClient::readResponseFromFile() const {
	if (!fs::exists(RESPONSE)) {
		return false;
	}
	
	while(fs::exists(RESPONSE_LOCK)) {
		sleep(1);
	}
	std::ofstream lock{ RESPONSE_LOCK, std::ios::out | std::ios::trunc };
	if (!lock.is_open()) {
		throw std::runtime_error{ "Error: can not create response lock file" };
	}
	lock.close();
	std::ifstream is{ RESPONSE, std::ios::in };
	std::string response;
	getline(is, response);
	strcpy(message_, response.c_str());

	fs::remove(RESPONSE);
	fs::remove(RESPONSE_LOCK);
	return true;
}

void ChatClient::signOut() {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}

	strcpy(message_, "/logout");
	sendRequest();
	loggedUser_.reset();
}

void ChatClient::removeUser(ChatUser& user) {
	if (loggedUser_ == nullptr) {
		std::cout << "You are not logged in\n" << std::endl;
		return;
	}
	strcpy(message_, "/remove");
	sendRequest();
	receiveResponse();
	if (strncmp(message_, "/response:fail", 14) == 0) {
		std::cout << "Some issue occured while removing current user on server. Try again later" << std::endl;
	}
	else {
		std::cout << "User removed successfully\n" << std::endl;
		loggedUser_.reset();
	}
}

ssize_t ChatClient::sendRequest() const {
	if (*message_ == '\0') {
		// invalid argument passed
		throw std::invalid_argument("Message cannot be empty");
	}
	ssize_t bytes = write(sockFd_, message_, MESSAGE_LENGTH);

	return bytes;
}

ssize_t ChatClient::receiveResponse() const {
	ssize_t bytes = read(sockFd_, message_, MESSAGE_LENGTH);
	std::cout << "Received response: " << message_ << std::endl;
	return bytes;
}

void ChatClient::printPrompt() const {
	std::cout << (loggedUser_ ? loggedUser_->getLogin() : "") << "> ";
}

void ChatClient::work() {
	while (true) {
		try {
			printPrompt();
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
				sendRequest();
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
		}
		catch (std::invalid_argument e) {
			// exception handling
			std::cout << "Error: " << e.what() << "\n" << std::endl;
		}
	}

	cleanExit();
}

void ChatClient::clearPrompt() const {
	// Using ANSI escape sequence CSI2K and then carriage return
	std::cout << "\x1B[2K\r";
	std::cout.flush();
}

void ChatClient::printSystemInformation() const {
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
