#pragma once
#include <iostream>
#include <string>

class chat_user
{
public:
	// construct
	chat_user(const std::string& login, const std::string& password, const std::string& name);

	// getters
	const std::string& getLogin() const;
	const std::string& getPassword() const;
	const std::string& getName() const;
	
	// setters
	void setAuthorized(bool value);

	// authorization flag
	bool isAuthorized() const;

private:
	std::string login;
	std::string password;
	std::string name;
	bool authorized;
};