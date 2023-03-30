#include "ChatSave.hpp"
#include "ChatRoomClient.hpp"
#include "Display.hpp"
#include "spdlog/fmt/bundled/chrono.h"
#include "spdlog/spdlog.h"
#include <ctime>
#include <exception>
#include <fstream>
#include <ios>
#include <ostream>
#include <shared_mutex>
#include <string>

void ChatSL::save(const std::string &addr, std::string msg, char type,
                  std::time_t timeStamp, const std::string &title) {
  if (!chatRoomClient.hasUsr(addr)) {
    spdlog::error("No such file exit.");
    return;
  }
  auto &c = chatRoomClient.usrs[addr];
  // std::unique_lock<std::mutex> lock(*c.first.get());
  std::fstream file("chatSave/" + addr,
                    std::ofstream::app | std::ofstream::out | std::ios::in);
  std::string fmtTime =
      fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(timeStamp));
  spdlog::info("Save msg from {}, title {}, content {}", addr, title, msg);
  //公告为B //广播为e
  if (type == 'B'||type=='e') {
    //"B"+"IP"
    //"e"+"IP"
    file << type << title << "\n";
  } else {
    file << type << addr << "\n";
  }
  spdlog::info("{}", msg);
  file << fmtTime << "\n";
  file << msg << std::endl;
  c.second->notify_all();
}

auto ChatSL::load(const std::string &addr)
    -> std::vector<std::shared_ptr<ChatBubble>> {
  auto fileName = "chatSave/" + addr;
  std::fstream file(fileName, std::ios::out | std::ios::in);

  std::string title, timeStamp, msg;
  std::vector<std::shared_ptr<ChatBubble>> bubbles;
  while (1) {
    if (!std::getline(file, title)) {
      break;
    }
    char tp = title[0];
    //代表消息由你发出
    if (title[0] == 'u') {
      //标题就是你
      title = "You";
      //消息由其他客户端发给你
    } else if (title[0] == 't') {
      //标题是他的地址
      title = addr;
      //从服务器来的公告
    } else if (title[0] == 'B') {
      //标题由服务器决定
      title = title.substr(1);
      //广播
    } else if (title[0] == 'e') {
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
    bubbles.push_back(BubbleFactory::bubbleMaker(tp, title, timeStamp, msg));
  }
  return bubbles;
}