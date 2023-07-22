#pragma once
#include <iostream>
#include <string>

class chater
{
public:
	// construct
	chater(const std::string& login) : login(login) {}

	//Getter
	const std::string& getLogin() const { return login; }

private:
	std::string login;
};

