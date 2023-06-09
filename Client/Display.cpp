#include "Display.hpp"
#include "../Configuration.hpp"
#include "ChatRoomClient.hpp"
#include "ChatSave.hpp"
#include "spdlog/fmt/bundled/chrono.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <atomic>
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
std::atomic_bool canJump = true;

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
  // From cmb broadcast
  case 'e': {
    auto ret = std::make_shared<InBubble>(msg, "");
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    ret->inAddr = title;
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
  if (canJump) {
    fmt::print("\b\b");
    setcontext(&Interact::contextCmd);
  }
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
        ServerBubble("No target user", "Ip error").print();
        continue;
      }
      CurrentChatting = ip;
      cmdMode = false;
      showChat(ip);
    } else if (word == "Send" || word == "send") {
      if (!chatRoomClient.hasUsr(CurrentChatting)) {
        ServerBubble("No target user", "Ip error").print();
        continue;
      }
      if (str.size() < 5) {
        ServerBubble("Send what?", "Client msg").print();
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
      for (auto &&i : *chatRoomClient.usrs) {
        if (i.first == "cmb" || i.first == "server") {
          continue;
        }
        fmt::print("{} ", i.first);
      }
      lock.unlock();
      fmt::print("\nYou can chat with them.\n");
    } else if (word == "back") {
      showChat(CurrentChatting);
    } else if (word == "quit") {
      std::terminate();
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
  fmt::print("First of all, use key \'Ctrl+\\\' to reach command mode where "
             "the command followed should be entered.\n ");
  fmt::print("Enter \'chat $target ip$\' to open a chat.\n ");

  fmt::print("For example: \"Chat 192.169.1.1\"\n");
  fmt::print("enter \'cmb\' to go to group chat.\n");
  fmt::print("Enter \'Send $WHAT YOU WANT TO SAY$\', the massage will send to "
             "your last chat.\n");
  fmt::print("enter \'user\' to see who is online now.");
  fmt::print("enter \'back\' to back to chat room.\n");
  fmt::print("Now, goto command mode.\n");
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
  auto c = (*chatRoomClient.usrs)[ip];
#ifndef DEBUG
  while (1)
#endif // DEBUG
  {
    std::unique_lock<std::mutex> lock(*c.first);
    canJump = false;
#ifndef DEBUG
    system("clear");
#endif
    auto chatBubbles = chatSL.load(ip);
    CurrentChatting = ip;
    for (auto &&i : chatBubbles) {
      i->print();
    }
    canJump = true;
#ifndef DEBUG
//loop to avoid spurious wakeups 
    while (!*c.second.first) {
      c.second.second->wait(lock);
    }
    *c.second.first = false;
#else
    lock.unlock();
#endif
  }
#ifdef DEBUG
  setcontext(&Interact::contextCmd);
#endif
}

Interact::Interact() { signal(SIGQUIT, sigQuitHandler); }