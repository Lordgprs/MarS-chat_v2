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
	void save(const std::string &filename) const;

private:
	std::string login_;
	std::string password_;
	std::string name_;
};
