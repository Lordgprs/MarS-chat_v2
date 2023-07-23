#pragma once
#include <iostream>
#include <string>

class chat_user
{
public:
	// construct
	chat_user(const std::string& login, const std::string& password, const std::string& name)
		: login(login), password(password), name(name), authorized(false) {}

	//Getters
	const std::string& getLogin() const { return login; }
	const std::string& getPassword() const { return password; }
	const std::string& getName() const { return name; }

	bool isAuthorized() const
	{
		return authorized;
	}

	//Setters
	void setAuthorized(bool value)
	{
		authorized = value;
	}

private:
	std::string login;
	std::string password;
	std::string name;
	bool authorized;
};

