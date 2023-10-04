#include "chat_user.h"

#include <fstream>
#include <stdexcept>

// construct
ChatUser::ChatUser(const std::string& login, const std::string& password, const std::string& name)
	: login_{ login }, password_{ password }, name_{ name } {}

// Getters
const std::string& ChatUser::getLogin() const { return login_; }
const std::string& ChatUser::getPassword() const { return password_; }
const std::string& ChatUser::getName() const { return name_; }

void ChatUser::save(const std::string &filename) const {
	std::ofstream file(filename, std::ios::out | std::ios::app);
	if (!file.is_open()) {
		throw std::runtime_error{ "Cannot open file " + filename + " for append" };
	}

	file <<
		login_ << '\n' <<
		password_ << '\n' <<
		name_ << std::endl;
	file.close();
}

