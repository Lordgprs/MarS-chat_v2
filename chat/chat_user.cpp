#include "chat_user.h"

	// construct
	ChatUser::ChatUser(const std::string& login, const std::string& password, const std::string& name)
		: login(login), password(password), name(name) {}

	//Getters
	const std::string& ChatUser::getLogin() const { return login; }
	const std::string& ChatUser::getPassword() const { return password; }
	const std::string& ChatUser::getName() const { return name; }