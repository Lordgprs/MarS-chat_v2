#include "broadcast_message.h"

#include <iostream>

broadcast_message::broadcast_message(const std::string &sender, const std::string &text, const std::map<std::string, chat_user> &user_list) :
	users_unread_{ user_list } {
	sender_ = sender;
	text_ = text;
}

void broadcast_message::print() const {
	std::cout << sender_ << ": " << text_ << std::endl;
}

void broadcast_message::printIfUnreadByUser(const std::string &user) {
	if (users_unread_.find(user) != users_unread_.end()) {
			users_unread_.erase(user);
			print();
	}
}

bool broadcast_message::isRead() const {
	return users_unread_.empty();
}
