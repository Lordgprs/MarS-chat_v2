#include "broadcast_message.h"
#include <iostream>

BroadcastMessage::BroadcastMessage(const std::string &sender, const std::string &text, const std::map<std::string, ChatUser> &user_list) :
	users_unread_{ user_list } {
	sender_ = sender;
	text_ = text;
}

void BroadcastMessage::print() const {
	std::cout << sender_ << ": " << text_ << std::endl;
}

void BroadcastMessage::printIfUnreadByUser(const std::string &user) {
	if (users_unread_.find(user) != users_unread_.end()) {
			users_unread_.erase(user);
			print();
	}
}

bool BroadcastMessage::isRead() const {
	return users_unread_.empty();
}