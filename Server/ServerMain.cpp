#define SERVER
#include "../Configuration.hpp"
#include "Server.hpp"

int main() {

  auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      "ChatRoomLogger", true);
  auto serverSink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();
  spdlog::set_default_logger(std::make_shared<spdlog::logger>(
      "Server", (spdlog::sinks_init_list){serverSink, fileSink}));
  Server.run();
}