#ifndef DISPLAY_HPP
#define DISPLAY_HPP

#include "spdlog/spdlog.h"
#include <memory>
#include <string>
void onTTYChangeHandler(int);
class Display {
public:
  Display();
  void renderAndInteract();
  static void SetScrWidthAndMsgLen();
  inline static unsigned int scrWidth = 0;
  inline static unsigned short msgBoundLen = 0;

protected:
  inline static float msgBoundRatio = .7;
} inline display;

class ChatBubble {

public:
  friend Display;
  ChatBubble() = delete;
  ChatBubble(const std::string &str);
  virtual void print() = 0;
  virtual ~ChatBubble() {}
  std::string timeStamp;
  bool loadFromFile;

protected:
  inline static char buffer[2048];
  std::string msg;
  void genLeadingSpace();
  void genBoxBar();
  void cBuf();
  void genMsg(std::string sv);
  void genMsg(std::string_view sv);
  void genSL();
  void genEndingSpace();
  std::string getTimeNow();
};

class UsrBubble : public ChatBubble {
public:
  UsrBubble(const std::string &str);
  void print() override;
};

class ServerBubble : public ChatBubble {
public:
  ServerBubble(const std::string &str, const std::string &title);
  void print() override;

private:
  const std::string title;
};

class InBubble : public ChatBubble {
public:
  InBubble(const std::string &str, const std::string &from);
  void print() override;
  std::string inAddr;

};

class BubbleFactory {
public:
  friend std::shared_ptr<ChatBubble>;
  friend ChatBubble;
  static auto bubbleMaker(const std::string &title,
                          const std::string &timeStamp, const std::string &msg,
                          const std::string scr = "")
      -> std::shared_ptr<ChatBubble>;
};

#endif