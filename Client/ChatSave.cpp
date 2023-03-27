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