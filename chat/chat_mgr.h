#pragma once
#include "chat_user.h"
#include <iostream>
#include <string>
#include <map>

class chat_mgr
{
public:
	// construct
	chat_mgr();

	// function help
	void displayHelp();

	// login availability
	bool isLoginAvailable(const std::string& login) const;

	// registration
	void signUp();

	// login verification
	bool isValidLogin(const std::string& login) const;

	// authorization
	chat_user *signIn();

	// user logout
	void signOut();

	// deleting a user
	void removeUser(chat_user& user);

	// sending a message
	void sendMessage(const std::string& message) const;

	// sending a private message
	void sendPrivateMessage(chat_user& sender, const std::string& receiverName, const std::string& messageText) const;

	// sending a shared message
	void sendBroadcastMessage(chat_user& sender, const std::string& message) const;

	// main work
	void work();

private:
	std::map<std::string, chat_user> users_;
	chat_user *loggedUser_;
};