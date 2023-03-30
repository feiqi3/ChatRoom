#pragma once
#include "spdlog/fmt/bundled/core.h"
#include <cstring>
#include <memory>
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
      : len(mt.token != 2 ? 2 + 20 + mt.msg.size() : 2 + mt.msg.size()),
        byte(std::shared_ptr<char[]>(
            new char[mt.token != 2 ? 2 + 20 + mt.msg.size()
                                   : 2 + mt.msg.size()])) {
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
      byte[0] = tok;
      break;
    }
    }

    memset(byte.get() + 1, '\0', len - 1);
    memcpy(byte.get() + 2, mt.addr.c_str(), mt.addr.size());
    memcpy(byte.get() + mt.addr.size() + 2 + 1, mt.msg.c_str(), mt.msg.size());
    byte[1 + mt.addr.size() + 1] = ' ';
    byte[1] = ' ';
  }
  ~MsgTokenByte() {}
  std::shared_ptr<char[]> byte;
  const int len;
};

char *genErrorMsg();

class MsgPairParserClient {
public:
  friend class ChatRoom;
  inline int parser(std::string &inAddr, const char *in, int len) {
    spdlog::trace("New thread {}", getpid());
    spdlog::info("receive message from {}, :\"{}\" ", inAddr, in);

    char token = in[0];
    // 广播信息
    if (token == '2') {
      std::string msg = std::string(in + 2);
      //组装一个广播信息
      //格式是： Broadcast srcIP msg
      MsgToken msgToken(MsgToken::Broadcast, msg, inAddr);
      MsgTokenByte tokenByte(msgToken);
      chatRoom.msgBroadcast(inAddr, tokenByte.byte, tokenByte.len);
      return 1;
      //组装一个端对端信息
      //格式是 From scrIp msg
    } else if (token == '0') {
      int iplen;
      auto tar = getIp(in, iplen, len);
      spdlog::info("tar IP: {}",tar);
      if (!chatRoom.isUserExist(tar)) {
        if (errorHandler(inAddr, tar, NO_TARGET_CLIENT_ERROR) < 0) {
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
        if (errorHandler(inAddr, tar, CLIENT_CLOSED_ERROR) < 0) {
          return -1;
        }
      }
      return 1;
    } else if (token == '3') {
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
    int flag = 0;
    while (1) {
      std::stringstream ss;
      int len = 0;
      for (; len < 1400; ++i, len += 20) {
        if (i == chatRoom.UserConns.end()) {
          flag = 1;
          break;
        }
        ss<<" "<< i->first << " ";
      }
      svec.push_back(ss.str());
      if (flag)
        break;
    }
    lock.unlock();
    //发送IP
    for (auto &&si : svec) {
      MsgToken msg((MsgToken::Token)'I', si, nullString);
      MsgTokenByte byte(msg);
      try {
        chatRoom.msgSend(serverString, in, byte.byte, byte.len);
      } catch (badSending) {
        chatRoom.eraseUserMap(in);
        return -1;
      }
    }
    // end token
    MsgToken msg(MsgToken::Token('N'), "N", serverString);
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
    memset(ip, '\0', 20);
    memcpy(ip, in + 2, ipLen);
    std::string addr(ip);
    return addr;
  }

  int errorHandler(const std::string &inAddr, const std::string &tar,
                   const std::string &errmsg) {
    //组装一条错误信息
    //格式是 Note scrIp note
    //其中Ip是聊天时来源的ip
    //广播时不会发送错误
    //只有端对端才有
    MsgTokenByte msgErr(MsgToken(MsgToken::Note, errmsg, tar));
    try {
      chatRoom.msgSend(tar, inAddr, msgErr.byte, msgErr.len);
    } catch (badSending) {
      return -1;
    }
    spdlog::info("It looks like two client all closed. Sending cancelled.");
    return 1;
  }
} inline serverParser;

#endif