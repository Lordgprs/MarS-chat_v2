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

class ChatMgr final {
public:
	// construct
	ChatMgr();

	// function help
	void displayHelp();

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

	// main work
	void work();

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


private:
	std::map<std::string, ChatUser> users_;
	std::vector<std::shared_ptr<ChatMessage>> messages_;
	ChatUser *loggedUser_{ nullptr };
	bool usersFileMustBeUpdated_ { false };
	const std::string USER_CONFIG{ "users.cfg" };
	const std::string MESSAGES_LOG{ "messages.log" };
};
