#include "Connection.hpp"
#include "spdlog/spdlog.h"
#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <cerrno>
#include <cstring>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>

Connection::Connection(const std::string &addr, uint16_t port, bool hasBuf)
    : addrStr(addr), addr(genAddr(addr, port)), hasbuf(hasBuf) {
  if (hasBuf) {
    buf = new char[bufSize];
  }
  if ((conSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    printf("Socket setup error: %s\n", std::strerror(errno));
    fflush(stdout);
    exit(1);
  }
}
Connection::Connection(in_addr_t inAddr, uint16_t port, bool hasBuf)
    : hasbuf(hasBuf) {
  if (hasBuf) {
    buf = new char[bufSize];
  }
  char ip[20];
  inet_ntop(AF_INET, &inAddr, ip, sizeof(inAddr));
  addrStr = std::string(ip);
  addr.sin_addr.s_addr = htonl(inAddr);
  addr.sin_family = AF_INET;
  addr.sin_port = port;

  if ((conSock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    spdlog::error("Socket setup error: {}\n", strerror(errno));
    fflush(stdout);
    exit(1);
  }
}

sockaddr_in Connection::genAddr(const std::string &addr, const int port) {
  struct sockaddr_in _addr;
  _addr.sin_family = AF_INET;
  _addr.sin_port = htons(port);
  assert(inet_pton(AF_INET, addr.c_str(), &_addr.sin_addr) >= 0);
  return _addr;
}

void Connection::open() {
  if (connect(conSock, (sockaddr *)&addr, sizeof(addr)) < 0) {
    if (errno == EISCONN) {
      spdlog::debug("Connecting to a connected socket, address: {}", addrStr);
      return;
    }
    throw badConnection(addrStr);
  }
}

void Connection::bind() {
  spdlog::info("Bind Server to port: {}", addr.sin_port);
  if (::bind(conSock, (sockaddr *)&addr, sizeof(addr)) < 0) {
    throw badBinding();
  }
}
Connection::Connection(sockfd infd, sockaddr_in &&in, bool hasBuf)
    : conSock(infd), addr(in), hasbuf(hasBuf) {
  if (hasBuf) {
    buf = new char[bufSize];
  }
  char ip[20];
  inet_ntop(AF_INET, &in, ip, sizeof(in));
  addrStr = std::string(ip);
}

std::unique_ptr<Connection> Connection::accept(const Connection &listenConn) {
  sockaddr_in inSock;
  socklen_t tmp;
Flag_accept:
  auto fdin = ::accept(listenConn.getSock(), (sockaddr *)&inSock, &tmp);
  if (fdin < 0) {
    //慢系统调用，指的是可能永远无法返回，从而使进程永远阻塞的系统调用，比如无客户连接时的accept、无输入时的read都属于慢速系统调用。
    //在Linux中，当阻塞于某个慢系统调用的进程捕获一个信号，则该系统调用就会被中断，转而执行信号处理函数，这就是被中断的系统调用。
    if (errno == EINTR) {
      spdlog::warn("'EINT' error occurred during acception. Retry.");
      goto Flag_accept;
    }
    throw badAcception();
  }
  auto ret = std::make_unique<Connection>(fdin, std::move(inSock),true);
  return ret;
}

void Connection::listen() {
  spdlog::info("Listening port {}", addr.sin_port);
  if (::listen(conSock, backlog) < 0) {
    throw badListening();
  }
}

void Connection::send(char *buf, int len) {
  // About MSG_NOSIGNAL
  //如果遇到乐关闭的socket
  // linux会发送一个SIGPIPE信号
  //这个信号会关闭当前线程
  spdlog::debug("Sending data in thread {}, target: {}", getpid(), getAddr());
  if (::send(conSock, buf, len, MSG_NOSIGNAL) >= 0) {
    return;
  } else {
    throw badSending(addrStr);
  }
}

void Connection::recv() {
  _mutex.lock();
  memset(buf, '\0', bufSize);
  int ret;
  if ((ret = ::recv(conSock, buf, bufSize - 1, MSG_NOSIGNAL)) > 0) {
    _mutex.unlock();
    return;
  }
  _mutex.unlock();
  throw badReceiving(addrStr, ret);
}

Connection::~Connection() {
  close(conSock);
  if (hasbuf)
    delete[] buf;
}

Connection::Connection(Connection &&conn)
    : buf(conn.buf), hasbuf(conn.hasbuf), conSock(conn.conSock),
      addr(conn.addr), addrStr(conn.addrStr) {}