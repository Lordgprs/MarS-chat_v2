#pragma once
#include "chat_user.h"
#include "chat_message.h"
#include "broadcast_message.h"
#include "private_message.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#if defined(_WIN64) or defined(_WIN32)
#include <Windows.h>

struct WindowsVersion {
	unsigned major, minor, build;
};
#endif

class ChatMgr final {
public:
	// construct
	ChatMgr();
	
	// main work
	void work();

	// function help
	void displayHelp();

private:
	// login availability
	bool isLoginAvailable(const std::string& login) const;

	// registration
	void signUp();

	// login verification
	bool isValidLogin(const std::string& login) const;

	// authorization
	void signIn();

	// user logout
	void signOut();

	// deleting a user
	void removeUser(ChatUser& user);

	// sending a message
	void sendMessage(const std::string& message);

	// sending a private message
	void sendPrivateMessage(ChatUser& sender, const std::string& receiverName, const std::string& messageText);

	// sending a shared message
	void sendBroadcastMessage(ChatUser& sender, const std::string& message);

	// check unread messages
	void checkUnreadMessages();
	
	// save all users (i. e. after removing some of them) to file
	void saveUsers() const;

	// save all messages to file
	void saveMessages() const;

	// load user list from file
	void loadUsers();

	// load message list from file
	void loadMessages();

	// print information about process and OS
	void printSystemInformation() const;

	// Get literal version, i.e. 5.0 is Windows 2000
	std::string getLiteralOSName(OSVERSIONINFOEX &osv) const;

	std::map<std::string, ChatUser> users_;
	std::vector<std::shared_ptr<ChatMessage>> messages_;
	ChatUser *loggedUser_{ nullptr };
	bool usersFileMustBeUpdated_ { false };
	const std::string USER_CONFIG{ "users.cfg" };
	const std::string MESSAGES_LOG{ "messages.log" };
};
