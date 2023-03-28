#include "ChatSave.hpp"
#include "Display.hpp"
#include "spdlog/spdlog.h"
#include <ctime>
#include <fstream>
#include <ostream>
#include <string>

void ChatSL::save(const std::string &addr, std::string msg, char from,
                  std::time_t timeStamp) {
  std::ofstream file("chatSave/" + addr,
                     std::ofstream::app | std::ofstream::out);
  std::string fmtTime = fmt::format("{:%Y-%m-%d %H:%M:%S}\n", timeStamp);
  file << from << "\n";
  file << msg << "\n";
  file << fmtTime << std::endl;
}
void ChatSL::broadSave(const std::string &addr, std::string msg,
                       std::time_t timeStamp) {
  std::ofstream file("chatSave/World",
                     std::ofstream::app | std::ofstream::out);
  std::string fmtTime = fmt::format("{:%Y-%m-%d %H:%M:%S}\n", timeStamp);
  file << "e"+addr << "\n";
  file << msg << "\n";
  file << fmtTime << std::endl;
}
auto ChatSL::load(const std::string &addr)
    -> std::vector<std::shared_ptr<ChatBubble>> {
  auto fileName = "chatSave/" + addr;
  std::ifstream file(fileName);
  std::string title, timeStamp, msg;
  std::vector<std::shared_ptr<ChatBubble>> bubbles;
  while (1) {
    if (!std::getline(file, title)) {
      break;
    }
    //代表消息由你发出
    if (title[0] == 'u') {
    //标题就是你
      title = "You";
    //消息由其他客户端发给你
    } else if (title[0] == 't') {
    //标题是他的地址  
      title = addr;
    //从服务器来的公告
    }else if(title[0] == 'B'){
    //标题由服务器决定
      title = title.substr(1);
    //广播
    }else if(title[0] == 'e'){
    //标题更在第一个字符后面  
      title = title.substr(1);
    }
    if (!std::getline(file, timeStamp)) {
      spdlog::error("Chat file {} is broken.", fileName);
      break;
    }
    if (!std::getline(file, msg)) {
      spdlog::error("Chat file {} is broken.", fileName);
      break;
    }
    bubbles.push_back(BubbleFactory::bubbleMaker(title, timeStamp, msg));
  }
  return bubbles;
}