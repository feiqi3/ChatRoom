#pragma once
#include "../Connection.hpp"
#include <functional>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#define SERVERNAME "Server"

#define CLIENT_CLOSED_ERROR "Target client closed."
#define NO_TARGET_CLIENT_ERROR "No target client."

class ChatServer {
public:
  void run();

} inline Server;

class ChatRoom {
public:
  int toTarUser(std::string &addr, std::function<bool(Connection &tar)>);

  // When conn in, this function will do the rest of work
  void connHandler(std::unique_ptr<Connection> &&);
  int msgSend(const std::string &src,const std::string &dst, char *msg, size_t msglen);
  void msgBroadcast(std::string &src, char *msg, size_t msglen);
  std::unordered_map<std::string, std::shared_ptr<Connection>> UserConns;
  bool isUserExist(std::string &addr);

  mutable std::shared_mutex ioLock;
  std::shared_ptr<Connection> &writeUserMap(std::shared_ptr<Connection> &&conn);
  void eraseUserMap(const std::string &addr);


} inline chatRoom;