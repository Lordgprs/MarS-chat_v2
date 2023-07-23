#include "chat_user.h"

	// construct
	chat_user::chat_user(const std::string& login, const std::string& password, const std::string& name)
		: login(login), password(password), name(name), authorized(false) {}

	//Getters
	const std::string& chat_user::getLogin() const { return login; }
	const std::string& chat_user::getPassword() const { return password; }
	const std::string& chat_user::getName() const { return name; }

	bool chat_user::isAuthorized() const
	{
		return authorized;
	}

	//Setters
	void chat_user::setAuthorized(bool value)
	{
		authorized = value;
	}