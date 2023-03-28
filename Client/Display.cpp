#include "Display.hpp"
#include "../Configuration.hpp"
#include "spdlog/fmt/bundled/chrono.h"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <exception>
#include <memory>
#include <signal.h>
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/chrono.h>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <unistd.h>
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
  std::string time = loadFromFile ? timeStamp : getTimeNow();
  fmt::print("┌{0:─^{2}}┐\n"
             "│{3: ^{2}}│\n"
             "│{4: ^{2}}│\n"
             "│{1: ^{2}}│\n"
             "└{0:─^{2}}┘\n",
             "", msg, Display::msgBoundLen, time, title);
}

auto BubbleFactory::bubbleMaker(const std::string &title,
                                const std::string &timeStamp,
                                const std::string &msg, const std::string scr)
    -> std::shared_ptr<ChatBubble> {
  char tp = title[0];
  switch (tp) {
  // B -> bulletin
  case 'B': {
    auto ret = std::make_shared<ServerBubble>(msg, title.substr(1));
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    return ret;
    break;
  }
  // C -> chat
  case 'C': {
    auto ret = std::make_shared<InBubble>(msg, scr);
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    ret->inAddr = title.substr(1);
    return ret;
    break;
  }

  case 'S': {
    auto ret = std::make_shared<UsrBubble>(msg);
    ret->timeStamp = timeStamp;
    ret->loadFromFile = true;
    return ret;
    break;
  }
  default: {
    spdlog::error("Chat file loading error");
    auto ret = std::make_shared<ServerBubble>(
        "Internal error occurred when loading chat file.", "Programme Error");
    ret->timeStamp =
        fmt::format("{:%Y-%m-%d %H:%M:%S}\n", std::time_t(nullptr));
    ret->loadFromFile = true;
    return ret;
  }
  }
}