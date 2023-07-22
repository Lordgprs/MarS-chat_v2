#include <iostream>
#include <string>
#pragma once

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

	void signup(const std::string& login, const std::string& password, const std::string& name)
	{
		if (password.empty() || name.empty()) {
			throw std::invalid_argument("Password and name cannot be empty.");
		}
	}
};

