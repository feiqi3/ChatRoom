#pragma once
#include <memory>
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
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <spdlog/common.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/bundled/os.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#define SERVER_IP INADDR_ANY

constexpr int MAX_THREAD = 128;
constexpr ushort SERVER_PORT = 6666;

class spdConfig {
public:
  spdConfig() {
    if (isInit) {
      return;
    }
    isInit = true;

    auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "ChatRoomLogger", true);

#ifdef SERVER
    auto serverSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    spdlog::set_default_logger(std::make_shared<spdlog::logger>(
        "Server", (spdlog::sinks_init_list){serverSink, fileSink}));
#else
    spdlog::set_default_logger(
        std::make_shared<spdlog::logger>("Server", fileSink));
#endif

    spdlog::flush_every(std::chrono::seconds(1));
    spdlog::set_level(spdlog::level::trace);
#ifdef PROD
    spdlog::flush_every(std::chrono::seconds(5));
    spdlog::set_level(spdlog::level::warn);
#endif
  }

private:
  inline static bool isInit = true;
} inline spdlogConfig; // Register

#endif