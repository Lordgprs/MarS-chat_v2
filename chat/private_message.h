#pragma once
#include "chat_message.h"
#include <string>

class private_message final : public chat_message {
public:
  private_message(const std::string &sender, const std::string &receiver, const std::string &text);

  // print the message
  void print() const override;

  // print the message if it is unread by user
  void printIfUnreadByUser(const std::string &) override;

  // check if message is not read
  bool isRead() const override;
private:
  bool read_{ false };
  std::string receiver_;
};