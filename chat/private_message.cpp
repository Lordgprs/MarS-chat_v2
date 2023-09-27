#include "private_message.h"
#include "chat_message.h"
#include "chat_user.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

PrivateMessage::PrivateMessage(const std::string &sender, const std::string &receiver, const std::string &text, const bool read) :
	receiver_{ receiver },
	read_ { read } {
	sender_ = sender;
	text_ = text;
}

void PrivateMessage::print() const {
	std::cout << sender_ << ": @" << receiver_ << " " << text_ << std::endl;
}

void PrivateMessage::printIfUnreadByUser(const std::string &user) {
	if (!read_ && (receiver_ == user)) {
		print();
		read_ = true;
	}
}

bool PrivateMessage::isRead() const {
	return read_;
}

void PrivateMessage::save(const std::string &filename) const {
	std::ofstream file(filename, std::ios::out | std::ios::app);
	if (!file.is_open()) {
		throw std::runtime_error{ "Error: cannot open file" + filename + " for append" };
	}
	file << "PRIVATE\n"
		<< sender_ << '\n'
		<< receiver_ << '\n'
		<< (read_ ? "READ" : "UNREAD") << '\n'
		<< text_ << std::endl;
	file.close();
}

