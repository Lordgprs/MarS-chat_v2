#include "private_message.h"
#include "chat_message.h"
#include "chat_user.h"
#include <memory>
#include <iostream>
#include <string>

PrivateMessage::PrivateMessage(const std::string &sender, const std::string &receiver, const std::string &text) :
  receiver_{ receiver } {
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