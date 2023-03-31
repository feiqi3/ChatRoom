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
  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "ChatRoomLogger", true);
  spdlog::set_default_logger(
      std::make_shared<spdlog::logger>("Client", fileSink));
  spdlogConfig;
  getcontext(&reTry);
  chatRoomClient.GetInfoFromServer();
  chatRoomClient.requestUserLists();
  interact.commandLine();

  return 0;
}