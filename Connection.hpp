#pragma once
#ifndef CONN
#define CONN
#include "./Configuration.hpp"
#include "spdlog/spdlog.h"
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>

using sockfd = int;

class badBinding : public std::exception {
public:
  badBinding() {
    spdlog::error("Failed to bind socket\nResult: {}", strerror(errno));
  }
  virtual const char *what() const throw() { return strerror(errno); }
};

class badConnection : public std::exception {
public:
  badConnection() = delete;
  badConnection(const std::string &_addr) : addr(_addr) {
    spdlog::error("Failed to connect to target: {}\nResult: {}", addr,
                  strerror(errno));
  }
  const std::string addr;
  virtual const char *what() const throw() { return strerror(errno); }
};

class badListening : public std::exception {
public:
  badListening() {
    spdlog::error("Failed to Listen the socket, result: {}", strerror(errno));
  }
  virtual const char *what() const throw() { return strerror(errno); }
};

class badAcception : public std::exception {
public:
  badAcception() {
    spdlog::error("Failed to accept request, result: {}", strerror(errno));
  }
  virtual const char *what() const throw() { return strerror(errno); }
};

class badSending : public std::exception {
public:
  badSending() = delete;
  badSending(const std::string &_addr) : addr(_addr) {
    spdlog::error("Failed to connect to target: {}\nResult: {}", addr,
                  strerror(errno));
  }
  const std::string addr;
  virtual const char *what() const throw() { return strerror(errno); }
};

class badReceiving : public std::exception {
public:
  badReceiving() = delete;
  badReceiving(const std::string &_addr, int retcode)
      : retCode(retcode), addr(_addr) {}
  const std::string addr;
  const int retCode;
  virtual const char *what() const throw() {
    spdlog::error("Failed to receive from target: {}\nResult: {}", addr,
                  strerror(errno));
    return strerror(errno);
  }
};

class Connection {
public:
  Connection(const std::string &addr, uint16_t port, bool hasBuf = false);
  Connection(in_addr_t inAddr, uint16_t port, bool hasBuf = false);
  Connection(sockfd infd, sockaddr_in &&in, bool hasBuf = true);
  Connection(Connection &&conn);
  Connection() = delete;
  inline char *getBuf() { return buf; }
  inline int getBufSize() { return bufSize; }

  inline sockfd getSock() const { return conSock; }
  inline std::string getAddr() { return addrStr; }
  inline sockaddr_in getSockAddr() { return addr; }
  static std::shared_ptr<Connection> accept(const Connection &listenConn);
  void open();
  void bind();
  void listen();
  void send(const char *buf, int len);
  void recv();
  inline static void setBacklog(int i) {
    if (i <= 0) {
      backlog = 1;
    }
    backlog = i;
  }
  // should be set before **every thing happens**
  inline static void setBufSize(int i) {
    if (i <= 200) {
      backlog = 200;
    } else if (i > 1500)
      backlog = 1500;
  }
  ~Connection();

protected:
  char *buf;
  sockfd conSock;
  sockaddr_in addr;
  std::string addrStr;
  const bool hasbuf;
  mutable std::mutex _mutex;

private:
  void genSocket() const;
  static sockaddr_in genAddr(const std::string &addr, const int port);
  inline static int backlog = 256;
  inline static int bufSize = 1500;
};

inline std::string addr2Str(sockaddr_in &inaddr) {
  char ip[20];
  inet_ntop(AF_INET, &inaddr, ip, sizeof(inaddr));
  return std::string(ip);
}

#endif