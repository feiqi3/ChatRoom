cmake_minimum_required(VERSION 3.17)


project(ChatRoomOnline)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")

file(GLOB BASIC "./*.cpp" "./*.hpp")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_subdirectory(Client)
add_subdirectory(Server)