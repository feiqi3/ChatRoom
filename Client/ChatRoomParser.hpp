#ifndef CLIENT_PARSER_HPP
#define CLIENT_PARSER_HPP

#include "ChatRoomClient.hpp"
#include "ChatSave.hpp"
#include "Display.hpp"
#include <condition_variable>
#include <cstring>
#include <ctime>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <utility>

extern bool cmdMode;

static auto getWord(const std::string &in, uint &ii, bool &isEnd)
    -> std::string;

class badParse : public std::exception {
public:
  badParse(std::string addr);
};

const std::string nullString("");
const std::string serverString("Server");
class MsgToken {
public:
  enum Token {
    To,
    From,
    Broadcast,
    Note,
  };
  const std::vector<std::string> tokens{"to", "From", "Broadcast", "Note"};
  MsgToken(Token t, const std::string &_msg, const std::string &_addr)
      : token(t), msg(_msg), addr(_addr) {}
  Token token;
  std::string addr;
  std::string msg;
};

class MsgTokenByte {
public:
  MsgTokenByte(const MsgToken &mt)
      : len(mt.token != 2 ? 2 + 20 + mt.msg.size() : 2 + mt.msg.size()) {
    byte = new char[len];
    auto msg = mt.msg;
    char tok = mt.token;
    switch (tok) {
    case MsgToken::To: {
      byte[0] = '0';
      break;
    }
    case MsgToken::From: {
      byte[0] = '1';
      break;
    }
    case MsgToken::Broadcast: {
      byte[0] = '2';
      break;
    }
    case MsgToken::Note: {
      byte[0] = '3';
      break;
    }
    default: {
      spdlog::error("Unexpected token.");
      byte[0] = '3';
      msg = "Error msg.";
      break;
    }
    }
    //清空
    memset(byte + 1, '\0', len - 1);
    byte[1] = ' ';
    //如果是广播，发送时不发送地址
    if (mt.token == mt.Broadcast) {
      //拷贝消息
      memcpy(byte + 2, mt.msg.c_str(), mt.msg.size());
    } else {
      //否则要发送地址
      byte[1 + mt.addr.size() + 1] = ' ';
      //拷贝地址
      memcpy(byte + 2, mt.addr.c_str(), mt.addr.size());
      //拷贝消息
      memcpy(byte + 2 + mt.addr.size() + 1, mt.msg.c_str(), mt.msg.size());
    }
  }
  ~MsgTokenByte() { delete[] byte; }
  char *byte;
  const int len;
};

class ParserClient {
public:
  void recParser(const char *in, int len) {
    int iplen;
    auto ip = getIp(in, iplen, len);
    // 1. from client
    if (in[0] == '1') {
      if (!chatRoomClient.hasUsr(ip))
        chatRoomClient.usrs[ip] = makeCVP();
      fromHandler(in, ip);
      // 2. Broadcast
    } else if (in[0] == '2') {
      broadHandler(in, ip);
      // 3. System note
    } else if (in[0] == '3') {
      noteHandler(in, ip);
    } else if (in[0] == 'I') {
      if (!chatRoomClient.usrListChange) {
        chatRoomClient.usrs.clear();
        chatRoomClient.usrs["cmb"] = makeCVP();
        chatRoomClient.usrs[serverString] = makeCVP();

        chatRoomClient.usrListChange = true;
      }
      getUserHandler(in);
    } else if (in[0] == 'N') {
      ServerBubble("User list has been updated.", "From client.").print();
      chatRoomClient.usrListChange = false;
    }
  }

private:
  void getUserHandler(const std::string &msg) {
    std::unique_lock<std::shared_mutex> lock(chatRoomClient.iolock);
    bool isEnd = false;
    uint ii = 1;
    std::string Server = getWord(msg, ii, isEnd);
    while (!isEnd) {
      std::string ip = getWord(msg, ii, isEnd);
      chatRoomClient.usrs[ip] = makeCVP();
    }
    return;
  }

  void fromHandler(const std::string &msg, const std::string &ip) {
    // 接受从端对段来的信息
    std::string tarMsg = msg.substr(2 + ip.size());

    chatSL.save(ip, tarMsg, 't', std::time(nullptr));
    if (ip != CurrentChatting || cmdMode == true) {
      ServerBubble("A new msg from " + ip, "Bing~").print();
    }
  }

  void noteHandler(const std::string &msg, const std::string &ip) {
    std::string tarMsg = msg.substr(2 + ip.size());
    chatSL.save(ip, tarMsg, 'B', std::time(nullptr), "Message from server.");
  }

  void broadHandler(const std::string &msg, const std::string &ip) {
    // 接受从广播来的信息
    std::string tarMsg = msg.substr(2 + ip.size());
    chatSL.save(ip, tarMsg, 'B', std::time(nullptr));
  }

  inline std::string getIp(const char *in, int &ipLen, int len) {
    for (ipLen = 0; ipLen < len - 3;) {
      if (in[ipLen + 2] != ' ') {
        ++ipLen;
      } else {
        break;
      }
    }
    char ip[20];
    memset(ip, '\0', 20);
    memcpy(ip, in + 2, ipLen);
    std::string addr(ip);
    return addr;
  }
} inline parserClient;

static auto getWord(const std::string &in, uint &ii, bool &isEnd)
    -> std::string {
  int ss = in.size();
  std::stringstream ssm;
  for (; ii < ss; ++ii) {
    if (in[ii] != ' ') {
      ssm << in[ii];
    } else {
      ++ii;
      break;
    }
  }
  if (ii >= ss) {
    isEnd = true;
  }
  return ssm.str();
}

#endif