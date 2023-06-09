#ifndef CHATROOMCLIENT_HPP
#define CHATROOMCLIENT_HPP

#include "../Configuration.hpp"
#include "../Connection.hpp"
#include "ChatSave.hpp"
#include "Display.hpp"
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <memory>
#include <mutex>
#include <set>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

//默认在大厅
inline std::string CurrentChatting = "cmb";

using CVP = std::pair<std::shared_ptr<std::mutex>,
                      std::pair<std::shared_ptr<std::atomic_bool>,std::shared_ptr<std::condition_variable>>>;
inline CVP makeCVP() {
  return CVP(std::shared_ptr<std::mutex>(new std::mutex),
             std::pair(std::shared_ptr<std::atomic_bool>(new std::atomic_bool(false)),std::shared_ptr<std::condition_variable>(new std::condition_variable())));
}
class ParserClient;

class ChatRoomClient {
  friend ParserClient;
  friend Interact;
  friend ChatSL;
public:
  ChatRoomClient();
  void GetInfoFromServer();
  void msgSend(const std::string &msg, std::string tarIp);
  void requestUserLists();
  bool hasUsr(std::string);
protected:
  bool usrListChange = false;
  std::shared_mutex iolock;
  std::thread recThread;
  void recHandler();
  std::unique_ptr<Connection> conn;
  // Has excetpion
  void establishConn();
  std::shared_ptr<std::unordered_map<std::string, CVP>> usrs;
} inline chatRoomClient;

#endif