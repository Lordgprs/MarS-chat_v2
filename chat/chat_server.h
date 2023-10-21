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
#include <set>
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
	void removeUser(); // deleting a user
	void sendMessage(); // sending a message
	void sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText); // sending a private message
	void sendBroadcastMessage(ChatUser& sender, const std::string& message); // sending a shared message
	void checkUnreadMessages(); // check unread messages
	void saveUsers() const;
	void saveMessages() const; // save all messages to file
	void loadUsers(); // load user list from file
	void loadMessages(const std::string &filename); // load message list from file
	void printSystemInformation() const; // print information about process and OS
	void printPrompt() const;
	unsigned int getPromptLength() const;
	void clearPrompt() const;
	void processNewClient();
	void startConsole();
	void checkLogin() const;
	void writeBuffer(const std::string &line) const;
	void terminateChild() const;
	void cleanExit();
	std::string getClientIpAndPort() const;
	void removeUserFromFile(const std::string &);
	void displayHelp() const;
	void writeUserFile() const;
	void removeUserFile(const std::string &username) const;
	void updateActiveUsers();
	void listActiveUsers();
	void kickClient(const std::string &cmd);
	void writeMessageToHistory(std::shared_ptr<ChatMessage> message) const;

	static const unsigned short MESSAGE_LENGTH{ 1024 };
	const std::string TEMP_DIR { "/tmp/chat_server" };
	const std::string USERS_DIR { TEMP_DIR + "/users" };
	const std::string USER_CONFIG{ "users.cfg" };
	const std::string HISTORY_LOG{ "messages.log" };
	const std::string HISTORY_LOCK{ TEMP_DIR + "/history.lock" };
	const std::string MESSAGE_LOCK{ USERS_DIR + "/messages.lock" };
	const std::string CONFIG_FILE{ "server.cfg" };
	const std::string PROMPT{ "server>" };
	const std::string BUFFER{ TEMP_DIR + "/buffer.tmp" };
	const std::string BUFFER_LOCK{ TEMP_DIR + "/buffer.lock" };
	const std::string USERLIST_LOCK{ TEMP_DIR + "/userlist.lock" };
	const int BACKLOG{ 5 };

#if defined(_WIN64) or defined(_WIN32)
	std::string getLiteralOSName(OSVERSIONINFOEX &osv) const; // Get literal version, i.e. 5.0 is Windows 2000
#endif

	std::map<std::string, ChatUser> users_;
	std::vector<std::shared_ptr<ChatMessage>> messages_;
	std::vector<ConfigFile> activeUsers_;
	std::string loggedUser_;
	ConfigFile config_{ CONFIG_FILE };
	sockaddr_in server_;
	sockaddr_in client_;
	int sockFd_;
	pid_t mainPid_;
	pid_t consolePid_;
	std::set<pid_t> children_;
	mutable char message_[MESSAGE_LENGTH];
	bool mainLoopActive_{ true };
	int connection_;
};

struct ClientInformation {
	std::string username;
	pid_t pid;
	std::string ip;
	unsigned short port;
};
