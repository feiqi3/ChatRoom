cmake_minimum_required(VERSION 3.13)


project(ChatRoomOnline)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g -pthread")

file(GLOB BASIC "./*.cpp" "./*.hpp")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(Client)
add_subdirectory(Server)