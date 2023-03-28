#ifndef CHATROOMCLIENT_HPP
#define CHATROOMCLIENT_HPP

#include "../Configuration.hpp"
#include "../Connection.hpp"
#include <memory>
#include <string>
#include <unordered_set>
class ChatRoomClient {
public:
  void GetInfoFromServer();
  void msgSend(const std::string& msg);
  
private:
  void recHandler();
  void sndHandler(const std::string& msg);
  std::unique_ptr<Connection> connRcv;
  std::unique_ptr<Connection> connSnd;
  //Has excetpion
  void establishConn() ;
}inline chatRoomClient;

#endif