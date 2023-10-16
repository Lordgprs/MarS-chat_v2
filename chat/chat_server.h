#pragma once
#include "chat_user.h"
#include "chat_message.h"
#include "broadcast_message.h"
#include "private_message.h"
#include "config_file.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstring>
#include <algorithm>
#include <stdexcept>

#if defined(_WIN64) or defined(_WIN32)
#include <Windows.h>
struct WindowsVersion {
	unsigned major, minor, build;
};
#elif defined(__linux__)
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

class ChatServer final {
public:
	ChatServer(); // constructor
	~ChatServer(); // destructor
	void work(); // main work
	void childDeathHandler(int signum);
	void sigIntHandler(int signum);
	void sigTermHandler(int signum);

private:	
	bool isLoginAvailable(const std::string& login) const; // login availability
	void signUp(); // registration
	bool isValidLogin(const std::string& login) const; // login verification
	void signIn(); // authorization
	void signOut(); // user logout
	void removeUser(ChatUser& user); // deleting a user
	void sendMessage(const std::string& message); // sending a message
	void sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText); // sending a private message
	void sendBroadcastMessage(ChatUser& sender, const std::string& message); // sending a shared message
	void checkUnreadMessages(); // check unread messages
	void saveUsers() const; // save all users (i. e. after removing some of them) to file
	void saveMessages() const; // save all messages to file
	void loadUsers(); // load user list from file
	void loadMessages(); // load message list from file
	void printSystemInformation() const; // print information about process and OS
	void printPrompt() const;
	unsigned int getPromptLength() const;
	void clearPrompt() const;
	void processNewClient(int connection);
	void startConsole() const;

	static const unsigned short MESSAGE_LENGTH{ 1024 };
	const std::string USER_CONFIG{ "users.cfg" };
	const std::string MESSAGES_LOG{ "messages.log" };
	const std::string CONFIG_FILE{ "server.cfg" };
	const std::string PROMPT{ "server>" };
	const int BACKLOG{ 5 };

#if defined(_WIN64) or defined(_WIN32)
	std::string getLiteralOSName(OSVERSIONINFOEX &osv) const; // Get literal version, i.e. 5.0 is Windows 2000
#endif

	std::map<std::string, ChatUser> users_;
	std::vector<std::shared_ptr<ChatMessage>> messages_;
	ChatUser *loggedUser_{ nullptr };
	bool usersFileMustBeUpdated_ { false };
	ConfigFile config_{ CONFIG_FILE };
	sockaddr_in server_;
	sockaddr_in client_;
	int sockFd_;
	pid_t mainPid_;
	pid_t consolePid_;
	char message_[MESSAGE_LENGTH];
	bool mainLoopActive_{ true };
};
