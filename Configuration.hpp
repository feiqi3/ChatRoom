#pragma once
#ifndef CONFIG
#define CONFIG
#include <arpa/inet.h>
#include <assert.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <netinet/in.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
constexpr int MAX_THREAD = 128;
constexpr ushort SERVER_PORT = 6666;
class spdConfig {
public:
  spdConfig() {
    if (isInit) {
      return;
    }
    isInit = true;

    auto chatFileLogger =
        spdlog::basic_logger_mt("ChatFileLogger", "ChatroomOnlineLogger");
        chatFileLogger->set_level(spdlog::level::trace);
    spdlog::register_logger(chatFileLogger);
#ifdef SERVER
    auto stdoutLogger = spdlog::stdout_color_mt("ServerLogger");
    spdlog::register_logger(stdoutLogger);
#endif
    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_level(spdlog::level::debug);
#ifdef PROD
    spdlog::flush_every(std::chrono::seconds(5));
    spdlog::set_level(spdlog::level::warn);
#endif
  }

private:
  inline static bool isInit = true;
} inline spdlogConfig; // Register

#endif