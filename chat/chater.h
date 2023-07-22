#pragma once
#include <iostream>
#include <string>

class chater
{
public:
	// construct
	chater(const std::string& login, const std::string& password) 
		: login(login), password(password), authorized(false) {}

	//Getter
	const std::string& getLogin() const { return login; }
	const std::string& getPassword() const { return password; }

	bool isAuthorized() const
	{
		return authorized;
	}

	void setAuthorized(bool value)
	{
		authorized = value;
	}

private:
	std::string login;
	std::string password;
	bool authorized;
};

