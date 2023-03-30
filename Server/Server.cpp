#include "Server.hpp"
#include "MsgTokenParser.hpp"
#include "spdlog/spdlog.h"
#include <cerrno>
#include <chrono>
#include <exception>
#include <memory>

void ChatServer::run() {
  Connection listenConn(INADDR_ANY, SERVER_PORT);
  try {
    listenConn.bind();
  } catch (badBinding) {
    spdlog::critical("Binding error. Critical error, program terminated.");
    exit(0);
  }

  try {
    listenConn.listen();
  } catch (badListening) {
    spdlog::critical("Listening error. Critical error, program terminated.");
    exit(0);
  }
  spdlog::info("Server is online, waiting for new connection.");
  while (1) {
    std::shared_ptr<Connection> inConn;
    try {
      inConn = listenConn.accept(listenConn);
    } catch (badAcception) {
      spdlog::critical("Accept error, Critical error, program terminated.");
      exit(0);
    }
    // chatRoom.connHandler(std::move(inConn));
    std::thread newThread(&ChatRoom::connHandler, &chatRoom, std::move(inConn));
    newThread.detach();
  }
}

void ChatRoom::connHandler(std::shared_ptr<Connection> conn) {
  // recv -> code == 0 , connection closed
  // code < 0 : error
  spdlog::info("New connection from {}", conn->getAddr());
  auto connTmp = writeUserMap(conn);
  auto addrIn = connTmp->getAddr();
  while (1) {
    try {
      connTmp->recv();
    } catch (badReceiving) {
      chatRoom.eraseUserMap(conn->getAddr());
      return;
    }
    spdlog::info("Start parsing msg {}", connTmp->getBuf().get());
    int ret =
        serverParser.parser(addrIn, connTmp->getBuf().get(), connTmp->getBufSize());
  }
  return;
}

std::shared_ptr<Connection>
ChatRoom::writeUserMap(std::shared_ptr<Connection> conn) {
  std::string addr = conn->getAddr();
  std::unique_lock<std::shared_mutex> lock(ioLock);
  return (UserConns[addr] = conn);
}

void ChatRoom::eraseUserMap(const std::string &addr) {
  std::unique_lock<std::shared_mutex> lock(ioLock);
  int t = UserConns.erase(addr);
  if (t > 0) {
    spdlog::info("Disconnecting user {}.", addr);
  }
  return;
}

bool ChatRoom::isUserExist(std::string &addr) {
  std::shared_lock<std::shared_mutex> lock(ioLock);
  return UserConns.contains(addr);
}

int ChatRoom::toTarUser(std::string &addr,
                        std::function<bool(Connection &tar)> func) {
  std::shared_lock<std::shared_mutex> lock(ioLock);
  if (!UserConns.contains(addr))
    // User does't exit
    return -1;
  auto tarAddr = UserConns[addr]->getSockAddr();
  int tmpFd = dup(UserConns[addr]->getSock());
  if (tmpFd < 0) {
    spdlog::warn("Dup connection failed.");
    return -1;
  }
  Connection subConn(tmpFd, std::move(tarAddr));
  lock.unlock();
  bool ret = func(subConn);
  if (ret == false) {
    eraseUserMap(addr);
    spdlog::info("Failed to connect to {}, connection closed.", addr);
  }
  return ret;
  // 0 operation fall
  // 1 operation succeed
}

int ChatRoom::msgSend(const std::string &src, const std::string &dst,
                      std::shared_ptr<char[]> msg, size_t msglen) {
  // spdlog::info("C2C: Msg from {} to {}", src, dst);
  spdlog::info("Msg from {} to {}: \"{}\"", src, dst, msg.get());
  std::shared_lock<std::shared_mutex> lock(ioLock);
  if (!UserConns.contains(dst))
    // User does't exit
    return -1;
  auto &tarConn = UserConns[dst];

  lock.unlock();
  //很明显send会阻塞，所以解除读写锁

  int connTimes = 0;
Flag_send:
  if (connTimes > 0) {
    std::this_thread::sleep_for(std::chrono::seconds(connTimes * 2));
    spdlog::info(
        "Failed to send message From {} to {}, have retryed for {} times.", src,
        dst, connTimes);
    if (connTimes > 3) {
      spdlog::info("Failed to connect to {}, connection closed.", dst);
      chatRoom.eraseUserMap(dst);
      return -1;
    }
  }
  try {
    tarConn->send(msg, msglen);
  } catch (badSending) {
    connTimes++;
    goto Flag_send;
  }
  return 1;
}

void ChatRoom::msgBroadcast(std::string &src, std::shared_ptr<char[]> msg,
                            size_t msglen) {

  spdlog::info("Broadcast: Msg from {} to all: {}", src, msg.get());
  std::vector<std::shared_ptr<Connection>> broadcastList;
  std::shared_lock<std::shared_mutex> lock(ioLock);
  for (auto &i : this->UserConns) {
    if (i.first == src) {
      continue;
    }
    auto tmp = i.second;
    broadcastList.push_back(tmp);
  }
  lock.unlock();
  std::vector<std::thread> broadcastThread;

  for (auto Conn : broadcastList) {
    std::shared_ptr<Connection> tmpConn(Conn);
    std::shared_ptr<char[]> tmpmsg(msg);
    std::thread sendMsg([](std::shared_ptr<Connection>&& tmpConn,std::shared_ptr<char[]>&& tmpmsg,int msglen) {
      try {
        tmpConn->send(tmpmsg, msglen);
      } catch (badSending) {
        return;
      }
    },std::move(tmpConn),std::move(tmpmsg),msglen);
    sendMsg.detach();
  }

  spdlog::trace("Broadcast from {} finished.", src);
  return;
}
