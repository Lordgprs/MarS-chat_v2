#pragma once
#include "SHA256.h"
#include "mysql.h"

#include <iostream>
#include <string>

class ChatUser final {
public:
	ChatUser(unsigned user_id, const std::string& login, const std::string& password, const std::string& name);

	// getters
	const std::string& getLogin() const;
	const std::string& getPassword() const;
	const std::string& getName() const;
	unsigned getUserId() const;
	bool isLoggedIn() const;
	void login();
	void logout();
	void save(Mysql &mysql) const;

private:
	std::string login_;
	std::string password_;
	std::string name_;
	unsigned user_id_;
	bool isLoggedIn_;
};

