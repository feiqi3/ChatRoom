#define CLIENT
#include "ChatRoomClient.hpp"
#include "../Configuration.hpp"
#include "../Connection.hpp"
#include "Display.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/os.h"
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>
#include "Display.hpp"

int main() {
  spdlogConfig;
  chatRoomClient.GetInfoFromServer();
  chatRoomClient.requestUserLists();
  interact.commandLine();

  return 0;
}