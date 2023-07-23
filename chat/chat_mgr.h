#pragma once
#include <iostream>
#include <string>
#include <map>
#include "chat_user.h"

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
	void signUp(const std::string& login, const std::string& password, const std::string& name);

	// login verification
	bool isValidLogin(const std::string& login) const;

	// authorization
	chat_user& signIn(const std::string& login, const std::string& password);

	// user logout
	void signOut(chat_user& user);

	// deleting a user
	void removeUser(chat_user& user);

	// sending a message
	void sendMessage(chat_user& sender, const std::string& message) const;

	// sending a private message
	void sendPrivateMessage(chat_user& sender, const std::string& receiverName, const std::string& messageText) const;

	// sending a shared message
	void sendBroadcastMessage(chat_user& sender, const std::string& message) const;

private:
	std::map<std::string, chat_user> users;
};