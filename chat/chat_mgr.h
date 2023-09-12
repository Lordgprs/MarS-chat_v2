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

private:
	std::map<std::string, ChatUser> users_;
	std::vector<std::shared_ptr<ChatMessage>> messages_;
	ChatUser *loggedUser_{ nullptr };
};