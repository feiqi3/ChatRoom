cmake_minimum_required(VERSION 3.13)

project(ChatRoomOnlineClient)
set(CMAKE_CXX_STANDARD 20)

file(GLOB CLIENT_SRC "*.cpp" "*.hpp")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

add_executable(${PROJECT_NAME} ${BASIC} ${CLIENT_SRC} )
target_include_directories(ChatRoomOnlineClient PUBLIC ../ext)
