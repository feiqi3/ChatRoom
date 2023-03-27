#ifndef CHATSAVE_HPP
#define CHATSAVE_HPP

#include "Display.hpp"
#include <../Configuration.hpp>
#include <string>
#include <unordered_map>
#include <vector>

class ChatSL {
public:
  void save(const std::string &addr, std::string msg, char from,
            std::time_t timeStamp);
  auto load(const std::string &addr)
      -> std::vector<std::shared_ptr<ChatBubble>>;
};

#endif