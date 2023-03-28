#ifndef CLIENT_PARSER_HPP
#define CLIENT_PARSER_HPP

#include "ChatRoomClient.hpp"
#include "ChatSave.hpp"
#include <ctime>
#include <string>

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
      byte[0] = 0;
      break;
    }
    case MsgToken::From: {
      byte[0] = 1;
      break;
    }
    case MsgToken::Broadcast: {
      byte[0] = 2;
      break;
    }
    case MsgToken::Note: {
      byte[0] = 3;
      break;
    }
    default: {
      spdlog::error("Unexpected token.");
      byte[0] = 3;
      msg = "Error msg.";
      break;
    }
    }

    if (tok != 3) {
      // except for note,other kind of msg ip followed by token
      memset(byte + 1, '\0', len - 1);
      memcpy(byte + 2, mt.addr.c_str(), mt.addr.size());
      memcpy(byte + 2 + mt.addr.size() + 2, mt.msg.c_str(), mt.msg.size());
    } else {
      memcpy(byte + 2, mt.msg.c_str(), mt.msg.size());
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
      fromHandler(in, ip);
    } else if (in[0] == '2') {
      broadHandler(in, ip);
    } else if (in[0] == 3) {
      fromHandler(in, ip);
    }
  }

private:
  void fromHandler(const std::string &msg, const std::string &ip) {
    // 接受从端对段来的信息
    std::string tarMsg = msg.substr(2 + ip.size());
    chatSL.save(ip, tarMsg, 't', std::time(nullptr));
  }

  void broadHandler(const std::string &msg, const std::string &ip) {
    // 接受从广播来的信息
    std::string tarMsg = msg.substr(2 + ip.size());
    chatSL.broadSave(ip, tarMsg, std::time(nullptr));
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
    memcpy(ip, in + 2, ipLen);
    std::string addr(ip);
    return addr;
  }
};

#endif