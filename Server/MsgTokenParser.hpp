#pragma once
#ifndef MSGTOKEN
#define MSGTOKEN
#include "Server.hpp"
#include "spdlog/spdlog.h"
#include "sstream"
#include <shared_mutex>
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
    if (mt.token != 3) {
      // note, ip followed by token
      memset(byte, 0, len);
      memcpy(byte + 2, mt.addr.c_str(), mt.addr.size());
      memcpy(byte + 2 + mt.addr.size() + 2, mt.msg.c_str(), mt.msg.size());
    } else {
      memcpy(byte + 2, mt.msg.c_str(), mt.msg.size());
    }
    switch (mt.token) {
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
      spdlog::critical("Unexpected token.");
      std::terminate();
    }
    }
  }
  ~MsgTokenByte() { delete[] byte; }
  char *byte;
  const int len;
};

char *genErrorMsg();

class MsgPairParserServer {
public:
  friend class ChatRoom;
  inline int parser(std::string &inAddr, const char *in, int len) {
    spdlog::trace("New thread {}", getpid());
    spdlog::trace("receive message from {}, :\"{}\" ", inAddr, in);
    char token = in[0];
    // Broadcast
    if (token == 2) {
      std::string msg = std::string(in + 2);
      //组装一个广播信息
      //格式是： Broadcast srcIP msg
      MsgToken msgToken(MsgToken::Broadcast, msg, inAddr);
      MsgTokenByte tokenByte(msgToken);
      chatRoom.msgBroadcast(inAddr, tokenByte.byte, tokenByte.len);
      return 1;
      // client to client
      // token in
    } else if (token == 0) {
      int iplen;
      auto tar = getIp(in, iplen, len);
      if (!chatRoom.isUserExist(tar)) {
        if (errorHandler(inAddr, CLIENT_CLOSED_ERROR) < 0) {
          return -1;
        }
        return 1;
      }
      const char *msgBeg = in + 1 + iplen + 2;
      std::string msg(msgBeg);
      //把to的token变成from的token
      //则to后的ip就是发送的目标
      //而源ip就是发送者
      auto token = MsgToken(MsgToken::From, msg, inAddr);
      auto msgbyte = MsgTokenByte(token);
      int ret = chatRoom.msgSend(inAddr, tar, msgbyte.byte, msgbyte.len);
      if (ret == -1) {
        if (errorHandler(inAddr, CLIENT_CLOSED_ERROR) < 0) {
          return -1;
        }
      }
      return 1;
    } else if (token == 3) {
      if (in[2] == 'I') {
        int ret = sendOnlineUsers(inAddr);
        if (ret < 0)
          return -1;
      }
    } else {
      spdlog::critical("Unexpected token from client {}.", inAddr);
      return 1;
    }
    return 1;
  }

private:
  // Note I ip1 ip2 .....

  inline int sendOnlineUsers(const std::string &in) {
    spdlog::info("Sending user list to {}", in);
    std::vector<std::string> svec;

    std::shared_lock<std::shared_mutex> lock(chatRoom.ioLock);
    auto &&i = chatRoom.UserConns.begin();
    //构造IP字符串
    while (1) {
      std::stringstream ss;
      ss << "I ";
      int len = 0;
      for (; i != chatRoom.UserConns.end() || len < 1400; ++i, len += 20) {
        ss << i->first;
      }
      svec.push_back(ss.str());
    }
    lock.unlock();
    //发送IP
    for (auto &&si : svec) {
      MsgToken msg(MsgToken::Note, si, serverString);
      MsgTokenByte byte(msg);
      try {
        chatRoom.msgSend(serverString, in, byte.byte, byte.len);
      } catch (badSending) {
        chatRoom.eraseUserMap(in);
        return -1;
      }
    }
    // end token
    MsgToken msg(MsgToken::Note, "N", serverString);
    MsgTokenByte byte(msg);
    try {
      chatRoom.msgSend(serverString, in, byte.byte, byte.len);
    } catch (badSending) {
      // Donothing
    }
    return 1;
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

  int errorHandler(const std::string &inAddr, const std::string &errmsg) {
    MsgTokenByte msgErr(MsgToken(MsgToken::Note, errmsg, serverString));
    try {
      std::string ServerName(SERVERNAME);
      chatRoom.msgSend(ServerName, inAddr, msgErr.byte, msgErr.len);
    } catch (badSending) {
      return -1;
    }
    spdlog::info("It looks like two client all closed. Sending cancelled.");
    return 1;
  }
} inline serverParser;

#endif