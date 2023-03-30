#include "ChatRoomClient.hpp"
#include "ChatRoomParser.hpp"
#include "ChatSave.hpp"
#include "Display.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/spdlog.h"
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <ucontext.h>
#include <unistd.h>
#include <utility>
void ChatRoomClient::GetInfoFromServer() {
  try {

    establishConn(); // Catch

  } catch (badConnection) {
    spdlog::critical("Failed to connect to Server.programme out.");
    std::terminate();
  }
  recThread = std::thread(&ChatRoomClient::recHandler, this);
  recThread.detach();
}

void ChatRoomClient::requestUserLists() { conn->send((char *)"3 I", 3); }

void ChatRoomClient::establishConn() {
  auto rcv = std::make_unique<Connection>(SERVER_IP, SERVER_PORT, true);
  rcv->open();
  this->conn = std::move(rcv);
}
bool ChatRoomClient::hasUsr(std::string ip) {
  std::shared_lock<std::shared_mutex> lock(iolock);
  return usrs.find(ip) != usrs.end();
}

void ChatRoomClient::recHandler() {
  while (1) {
    try {
      conn->recv();
      spdlog::info("Recv msg from server {}", conn->getBuf().get());
    } catch (badReceiving) {
      spdlog::critical("Receiving msg error");
      return;
    }
    parserClient.recParser(conn->getBuf(), conn->getBufSize());
  }
}

void ChatRoomClient::msgSend(const std::string &msg, std::string tarIp) {

  std::thread thread([msg, tarIp, this]() {
    try {
      MsgToken tk = tarIp == "cmb"
                        ? MsgToken(MsgToken::Broadcast, msg, nullString)
                        : MsgToken(MsgToken::To, msg, tarIp);
      MsgTokenByte byte(tk);
      this->conn->send(byte.byte, byte.len);
    } catch (badSending) {
      chatSL.save(tarIp, msg, 'B', std::time(nullptr), "Network error");
      fmt::print(
          "You can print 'Retry' in Command mode to reconnect to Server.");
      wait4Enter();
      setcontext(&Interact::contextCmd);
    }
  });
  thread.join();
}

ChatRoomClient::ChatRoomClient() { usrs["cmb"] = std::move(makeCVP()); }
