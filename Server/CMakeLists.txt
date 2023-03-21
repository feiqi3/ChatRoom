cmake_minimum_required(VERSION 3.17)

project(ChatRoomOnlineServer)
set(CMAKE_CXX_STANDARD 20)

file(GLOB SERVER_SRC "*.cpp" "*.hpp")

add_executable(${PROJECT_NAME} ${BASIC} ${SERVER_SRC} )
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

target_include_directories(ChatRoomOnlineServer PUBLIC ../ext)
