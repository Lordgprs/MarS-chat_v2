#pragma once
#include <iostream>
#include <string>
#include <map>
#include "chater.h"

class chat
{
public:
	// construct
	chat() {}

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
	bool isLoginAvailable(const std::string& login) const {
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

		users.emplace(login, chater(login));
	}

private:
	std::map<std::string, chater> users;
};