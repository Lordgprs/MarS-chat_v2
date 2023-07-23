#pragma once
#include <iostream>
#include <string>
#include <map>
#include "chat_user.h"

class chat_mgr
{
public:
	// construct
	chat_mgr() {}

	// function help
	void displayHelp() {
		std::cout << "Available commands:" << std::endl;
		std::cout << " /help - chat help, displays a list of commands to manage the chat" << std::endl;
		std::cout << " /signup - registration, user enters data for registration" << std::endl;
		std::cout << " /signin - authorization, only a registered user can authorize" << std::endl;
		std::cout << " /logout - user logout" << std::endl;
		std::cout << " /remove - delete registered user" << std::endl;
		std::cout << " /exit - close the program" << std::endl;
		std::cout << " @username is an addressable message," << std::endl;
		std::cout << "   not an addressable message for all users.\n" << std::endl;
	}

	// login availability
	bool isLoginAvailable(const std::string& login) const
	{
		return users.find(login) == users.end();
	}

	void signup(const std::string& login, const std::string& password, const std::string& name)
	{
		if (!isLoginAvailable(login)) {
			throw std::invalid_argument("The login you entered is already in use.");
		}

		if (login.empty() || password.empty()) {
			// invalid argument passed
			throw std::invalid_argument("Login or password cannot be empty.");
		}

		users.emplace(login, chat_user(login, password, name));
	}

	chat_user& signin(const std::string& login, const std::string& password)
	{
		auto id = users.find(login);
		if (id == users.end() || id->second.isAuthorized() || id->second.getLogin() != login || id->second.getPassword() != password) {
			throw std::invalid_argument("Invalid login or password.");
		}

		std::cout << id->second.getName() << " welcome to the chat!" << std::endl;

		id->second.setAuthorized(true);
		return id->second;
	}

	void signout(chat_user& user) {
		user.setAuthorized(false);
	}

private:
	std::map<std::string, chat_user> users;
};