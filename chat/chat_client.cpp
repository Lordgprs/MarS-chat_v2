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
	strcpy(message_, "/checklogin:");
	strcat(message_, login.c_str());
	sendMessage();
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
	sendMessage();
	receiveResponse();
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
	sendMessage();
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
	}
	else {
		std::cout << "Login failed" << std::endl;
	}
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

ssize_t ChatClient::receiveResponse() const {
	ssize_t bytes = read(sockFd_, message_, MESSAGE_LENGTH);
	std::cout << "Received response: " << message_ << std::endl;
	return bytes;
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
		}
		catch (std::invalid_argument e) {
			// exception handling
			std::cout << "Error: " << e.what() << "\n" << std::endl;
		}
	}
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
