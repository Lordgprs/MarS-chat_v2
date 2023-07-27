#pragma once
#include <iostream>
#include <string>

class chat_user final {
public:
	chat_user(const std::string& login, const std::string& password, const std::string& name);

	// getters
	const std::string& getLogin() const;
	const std::string& getPassword() const;
	const std::string& getName() const;

private:
	std::string login;
	std::string password;
	std::string name;
};