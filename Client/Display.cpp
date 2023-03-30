#include "Display.hpp"
#include "../Configuration.hpp"
#include "ChatRoomClient.hpp"
#include "ChatSave.hpp"
#include "spdlog/fmt/bundled/chrono.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <signal.h>
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/chrono.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <ucontext.h>
#include <unistd.h>
#include <utility>

bool cmdMode = false;


void Display::SetScrWidthAndMsgLen() {
  if (!isatty(STDOUT_FILENO)) {
    spdlog::critical("Please use programme in a Console!");
    std::terminate();
  }
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) < 0) {
    spdlog::critical("Cannot get tty size, programme out");
    std::terminate();
  }

  Display::scrWidth = ws.ws_col;
  int pWidth = (Display::scrWidth * Display::msgBoundRatio);
  if (pWidth < 30) {
    spdlog::warn("TTY too small");
  }
  Display::msgBoundLen = std::max(std::min(pWidth, 300), 30);
}

void ChatBubble::genEndingSpace() {
  int end = Display::scrWidth - Display::msgBoundLen;
  for (int i = 0; i < end; ++i) {
    fmt::print(" ");
  }
  fmt::print("\n");
}

void onTTYChangeHandler(int) { Display::SetScrWidthAndMsgLen(); }

Display::Display() {
  signal(SIGWINCH, onTTYChangeHandler);
  SetScrWidthAndMsgLen();
}

ChatBubble::ChatBubble(const std::string &in) : msg(in), loadFromFile(false) {}

UsrBubble::UsrBubble(const std::string &str) : ChatBubble(str) {}

ServerBubble::ServerBubble(const std::string &str, const std::string &_title)
    : title(_title), ChatBubble(str) {}

InBubble::InBubble(const std::string &str, const std::string &from)
    : ChatBubble(str), inAddr(from) {}

void ChatBubble::genLeadingSpace() {
  fmt::print("{:<{}}", "", Display::scrWidth - Display::msgBoundLen);
}
void ChatBubble::cBuf() { memset(buffer, '\0', 2048); }

void ChatBubble::genSL() { fmt::print("\n\n"); }

void ChatBubble::genBoxBar() {
  cBuf();
  memset(buffer, '-', Display::msgBoundLen);
  buffer[0] = '*';
  buffer[Display::msgBoundLen - 1] = '*';
  buffer[Display::msgBoundLen] = '\n';
  fmt::print("{}", buffer);
  cBuf();
}

void ChatBubble::genMsg(std::string_view sv) {
  cBuf();
  memset(buffer, ' ', Display::msgBoundLen - 2);
  assert(sv.size() <= Display::msgBoundLen - 2);
  sv.copy(buffer, sv.size());
}

void ChatBubble::genMsg(std::string sv) {
  cBuf();
  memset(buffer, ' ', Display::msgBoundLen - 2);
  assert(sv.size() <= Display::msgBoundLen - 2);
  sv.copy(buffer, sv.size());
}

std::string ChatBubble::getTimeNow() {
  std::time_t time = std::time(nullptr);
  std::string ret = fmt::format("{:%Y-%m-%d %H:%M:%S}", fmt::localtime(time));
  return ret;
}

void UsrBubble::print() {
  std::lock_guard<std::mutex> lockG(ChatBubble::lock);
  genLeadingSpace();
  ChatBubble::genBoxBar();
  int s = msg.size();
  std::string_view sv(msg);
  int len = (Display::msgBoundLen - 2);
  genLeadingSpace();
  genMsg((std::string_view) "You");
  fmt::print(bg(fmt::terminal_color::white), "|{}|\n", buffer);
  if (!loadFromFile)
    genMsg(getTimeNow());
  else
    genMsg(timeStamp);
  genLeadingSpace();
  fmt::print(bg(fmt::terminal_color::white), "|{}|\n", buffer);
  for (int i = 0; i < s / len + 1; ++i) {
    auto nSv = sv.substr(i * len, len);
    genMsg(nSv);
    genLeadingSpace();
    fmt::print(fg(fmt::color::floral_white) | bg(fmt::color::slate_gray),
               "|{}|\n", buffer);
  }
  genLeadingSpace();
  genBoxBar();
  genSL();
}

void InBubble::print() {
  std::lock_guard<std::mutex> lockG(ChatBubble::lock);
  ChatBubble::genBoxBar();
  int s = msg.size();
  std::string_view sv(msg);
  int len = (Display::msgBoundLen - 2);
  genMsg("From " + inAddr);
  fmt::print(bg(fmt::terminal_color::white), "|{}|", buffer);
  genEndingSpace();
  if (!loadFromFile)
    genMsg(getTimeNow());
  else
    genMsg(timeStamp);
  fmt::print(bg(fmt::terminal_color::white), "|{}|", buffer);
  genEndingSpace();
  for (int i = 0; i < s / len + 1; ++i) {
    auto nSv = sv.substr(i * len, len);
    genMsg(nSv);
    fmt::print(fg(fmt::color::floral_white) | bg(fmt::color::slate_gray),
               "|{}|", buffer);
    genEndingSpace();
  }
  genBoxBar();
  genSL();
}

void ServerBubble::print() {
  std::lock_guard<std::mutex> lockG(ChatBubble::lock);
  std::string time = loadFromFile ? timeStamp : getTimeNow();
  fmt::print("┌{0:─^{2}}┐\n"
             "│{3: ^{2}}│\n"
             "│{4: ^{2}}│\n"
             "│{1: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", msg, Display::msgBoundLen, time, title);
}

auto BubbleFactory::bubbleMaker(char tp, const std::string &title,
                                const std::string &timeStamp,
                                const std::string &msg, const std::string scr)
    -> std::shared_ptr<ChatBubble> {
  switch (tp) {
  // B -> bulletin
  case 'B': {
    auto ret = std::make_shared<ServerBubble>(msg, title.substr(1));
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    ret->title = title;
    return ret;
    break;
  }
  // t -> chat bubble from opposite
  case 't': {
    auto ret = std::make_shared<InBubble>(msg, scr);
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    ret->inAddr = title.substr(1);
    return ret;
    break;
  }

    // msg from usr
  case 'u': {
    auto ret = std::make_shared<UsrBubble>(msg);
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    return ret;
    break;
  }
  case 'n': {
    auto ret = std::make_shared<ServerBubble>(msg, "");
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    ret->title = title.substr(1);
    return ret;
    break;
  }
  default: {
    spdlog::error("Chat file loading error");
    auto ret = std::make_shared<ServerBubble>(
        "Internal error occurred when loading chat file.", "Programme Error");
    ret->timeStamp = fmt::format("{:%Y-%m-%d %H:%M:%S}\n",
                                 fmt::localtime(std::time_t(nullptr)));
    ret->loadFromFile = true;
    return ret;
  }
  }
}

void sigQuitHandler(int) {
  fmt::print("\b\b");
  setcontext(&Interact::contextCmd);
}

void Interact::commandLine() {
  getcontext(&contextCmd);
  cmdMode = true;
#ifndef DEBUG
  system("clear");
#endif
  fmt::print(bg(fmt::terminal_color::bright_blue),
             "┌{0:─^{2}}┐\n"
             "│{1: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", "Command Mode now", Display::scrWidth - 2);
  fmt::print(fmt::fg(fmt::terminal_color::blue),
             "Enter help for more detals.\n");
  while (1) {
    std::string in;
    std::getline(std::cin, in);

    InteractiveParser(in);
  }
}

static auto getWord(const std::string &in, uint &ii, bool &isEnd)
    -> std::string;
void helpHandler();
void Interact::InteractiveParser(const std::string &str) {
  bool isEnd = false;
  uint ii = 0;
  while (!isEnd) {
    fmt::print("\n\n");
    auto word = getWord(str, ii, isEnd);
    if (word == "help" || word == "Help") {
      cmdMode = false;

      helpHandler();
    } else if (word == "clear") {
      system("clear");
    } else if (word == "cmb" || word == "Chamber") {
      cmdMode = false;
      showChat("cmb");
    } else if (word == "chat" || word == "Chat") {
      auto ip = getWord(str, ii, isEnd);
      if (!chatRoomClient.hasUsr(ip)) {
        fmt::print("\n");
        ServerBubble("No target user", "Ip error").print();
        continue;
      }
      CurrentChatting = ip;
      cmdMode = false;
      showChat(ip);
    } else if (word == "Send" || word == "send") {
      if (!chatRoomClient.hasUsr(CurrentChatting)) {
        fmt::print("\n");
        ServerBubble("No target user", "Ip error").print();
        continue;
      }
      auto msg = str.substr(5);
      chatRoomClient.msgSend(str.substr(4), CurrentChatting);
      chatSL.save(CurrentChatting, msg, 'u', std::time(nullptr));
      cmdMode = false;
      showChat(CurrentChatting);
    } else if (word == "user") {
      chatRoomClient.requestUserLists();
      fmt::print("User online now:\n");
      std::shared_lock<std::shared_mutex> lock(chatRoomClient.iolock);
      for (auto &&i : chatRoomClient.usrs) {
        if (i.first == "cmb") {
          continue;
        }
        fmt::print("{} ", i.first);
      }
      lock.unlock();
      fmt::print("\n");
    } else {
      ServerBubble("Unknown command.", "Client msg").print();
    }
  }
}

void helpHandler() {
#ifndef DEBUG
  system("clear");
#endif
  fmt::print("┌{0:─^{2}}┐\n"
             "│{1: ^{2}}│\n"
             "│{3: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", "Manual4ChatRoom ", Display::msgBoundLen, "made by feiqi3");
  fmt::print("If U want to chat with someone, just enter \'To $ target ip $\' "
             "then follow by "
             "that guy's IP  ");

  fmt::print("For example: \"To 192.169.1.1\"\n"
             "Then U can reach to your own chatRoom.\n  Press Ctrl+\\ to ");
  fmt::print("go into command mode, where U can send msg\n"
             "enter 'Send $ the words you want to say $' \n\n");
  fmt::print("If U need to visit online group room, Just enter \'Chamber\' or "
             "\'cmb\' for short\n");
  wait4Enter();
  setcontext(&Interact::contextCmd);
  return;
}

static auto getWord(const std::string &in, uint &ii, bool &isEnd)
    -> std::string {
  int ss = in.size();
  std::stringstream ssm;
  for (; ii < ss; ++ii) {
    if (in[ii] == ' ') {
      continue;
    } else {
      break;
    }
  }
  for (; ii < ss; ++ii) {
    if (in[ii] != ' ') {
      ssm << in[ii];
    } else {
      break;
      ;
    }
  }
  if (ii >= ss) {
    isEnd = true;
  }
  return ssm.str();
}

void Interact::showChat(const std::string &ip) {
  //这个版本性能非常非常差
  //但是为了交作业无所谓了
  // TODO:
  // boost里有个可以分段加载文件的库，换成那个，每次只加载最后几条消息，性能可以高很多
  // TODO: 实现增量加载
  auto &c = chatRoomClient.usrs[ip];
  std::unique_lock<std::mutex> lock(*c.first);
#ifndef DEBUG
  while (1)
#endif // DEBUG
  {
#ifndef DEBUG
    // system("clear");
#endif
    auto chatBubbles = chatSL.load(ip);
    CurrentChatting = ip;
    for (auto &&i : chatBubbles) {
      i->print();
    }
#ifndef DEBUG
    c.second->wait(lock);
#endif
  }
  setcontext(&Interact::contextCmd);
}

Interact::Interact() { signal(SIGQUIT, sigQuitHandler); }