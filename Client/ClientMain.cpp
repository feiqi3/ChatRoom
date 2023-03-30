#include <sys/ucontext.h>
#include <ucontext.h>
#define CLIENT
#include "../Configuration.hpp"
#include "../Connection.hpp"
#include "ChatRoomClient.hpp"
#include "Display.hpp"
#include "spdlog/fmt/bundled/core.h"
#include "spdlog/fmt/bundled/os.h"
#include <cstdio>
#include <netinet/in.h>
#include <unistd.h>

ucontext_t reTry;

int main() {
  spdlogConfig;
  getcontext(&reTry);
  chatRoomClient.GetInfoFromServer();
  chatRoomClient.requestUserLists();
  interact.commandLine();

  return 0;
}