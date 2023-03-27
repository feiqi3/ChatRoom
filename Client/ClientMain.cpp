#include "../Configuration.hpp"
#include "../Connection.hpp"
#include "Display.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/os.h"
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>
#define SERVER_IP INADDR_ANY
int main() {
  Connection conn("SERVER_IP", SERVER_PORT, true);
  UsrBubble usrb("Hello");
  InBubble ib("ho", "192.168.1.1");
  ServerBubble sb("Connection down", "from server");
  ib.print();     
  usrb.print();    

  sb.print();
  /*   conn.open();
    char n[] = "3 I";
    conn.send(n, sizeof(n));
    conn.recv();
    std::printf("%s", conn.getBuf()); */
  return 0;
}