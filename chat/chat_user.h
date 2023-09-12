#pragma once
#include <iostream>
#include <string>

class ChatUser final {
public:
	ChatUser(const std::string& login, const std::string& password, const std::string& name);

	// getters
	const std::string& getLogin() const;
	const std::string& getPassword() const;
	const std::string& getName() const;

private:
	std::string login;
	std::string password;
	std::string name;
};