#include "chat_user.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

// construct
ChatUser::ChatUser(const unsigned user_id, const std::string& login, const std::string& password, const std::string& name)
	: login_{ login }, password_{ password }, name_{ name } {}

// Getters
const std::string& ChatUser::getLogin() const { return login_; }
const std::string& ChatUser::getPassword() const { return password_; }
const std::string& ChatUser::getName() const { return name_; }

void ChatUser::save(Mysql &mysql) const{
			std::stringstream ss;
			ss << "INSERT INTO `users` "
				"(`id`, `login`, `password_hash`, `name`)"
				"VALUES"
				"(" << user_id_ << ", '" << login_ << "', '" << password_ << "', '" << name_ << "');";
			mysql.query(ss.str());
}

void ChatUser::login() {
	isLoggedIn_ = true;
}

void ChatUser::logout() {
	isLoggedIn_ = false;
}

bool ChatUser::isLoggedIn() const {
	return isLoggedIn_;
}

unsigned ChatUser::getUserId() const {
	return user_id_;
}
