#include "ChatRoomClient.hpp"
#include "spdlog/spdlog.h"
#include <memory>
#include <utility>

void ChatRoomClient::GetInfoFromServer(){
    establishConn();//Catch
    
}

void ChatRoomClient::establishConn() {
    auto rcv = std::make_unique<Connection>(SERVER_IP,SERVER_PORT,true);
    rcv->open();
    this->connRcv = std::move(rcv);
    auto snd = std::make_unique<Connection>(dup(rcv->getSock()),SERVER_IP,true);
    snd->open();
}

void ChatRoomClient::recHandler(){
    while (1) {
        try {
        connRcv->recv();        
        }catch(badReceiving){
            spdlog::critical("Receiving msg error");
            return;
        }
        connRcv->getBuf();
    }
}