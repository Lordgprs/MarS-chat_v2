#pragma once
#include "chat_message.h"
#include "chat_user.h"
#include <map>
#include <string>

class broadcast_message final : public chat_message 
{
public:
  broadcast_message(const std::string &, const std::string &, const std::map<std::string, chat_user> &);

  // print the message
  void print() const override;

  // print the message if it is unread by user
  void printIfUnreadByUser(const std::string &) override;

  // check if message is read
  bool isRead() const override;

private:
  std::map<std::string, chat_user> users_unread_;
};