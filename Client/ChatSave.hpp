#ifndef CHATSAVE_HPP
#define CHATSAVE_HPP

#include "Display.hpp"
#include <../Configuration.hpp>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class ChatSL {
public:
//type: e-> broadcast; t-> chat msg from other; n-> note from server

  void save(const std::string &addr, std::string msg, char type,
            std::time_t timeStamp,const std::string& title = "");
  
  auto load(const std::string &addr)
      -> std::vector<std::shared_ptr<ChatBubble>>;
  
  protected:
  std::shared_mutex iolock;
  
} inline chatSL;

#endif