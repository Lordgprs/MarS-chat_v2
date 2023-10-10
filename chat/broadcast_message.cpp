#include "broadcast_message.h"
#include "project_lib.h"

#include <iostream>
#include <fstream>
#include <stdexcept>

BroadcastMessage::BroadcastMessage(
	const std::string &sender,
	const std::string &text,
	const std::map<std::string, ChatUser> &user_list
	) :
	users_unread_{ user_list } {
	sender_ = sender;
	text_ = text;
}

BroadcastMessage::BroadcastMessage(
	const std::string &sender, 
	const std::string &text,
	const std::map<std::string, ChatUser> &user_list,
	const std::string &user_list_str 
	) {
	sender_ = sender;
	text_ = text;

	auto users = Chat::split(user_list_str, ",");
	for (const auto &s: users) {
		auto it = user_list.find(s);
		if (it != user_list.end()) {
			users_unread_.emplace(it->first, it->second);
		}
	}
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

void BroadcastMessage::save(const std::string &filename) const {
	std::ofstream file(filename, std::ios::out | std::ios::app);
	if (!file.is_open()) {
		throw std::runtime_error{ "Error: cannot open file" + filename + " for append" };
	}
	file << "BROADCAST\n"
		<< sender_ << '\n';
	auto size = users_unread_.size();
	size_t i = 0;
	for (auto it = users_unread_.begin(); it != users_unread_.end(); ++it) {
		file << it->first;
		if (i < size - 1) {
			file << ',';
		}
		++i;
	}
	file << '\n' << text_ << std::endl;
	file.close();
}

