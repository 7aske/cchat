cmake_minimum_required(VERSION 3.13)
project(cchat C)

set(CMAKE_C_STANDARD 99)

add_executable(cchat source/client.c)
add_executable(cchat_server source/server.c)

TARGET_LINK_LIBRARIES(cchat pthread)
TARGET_LINK_LIBRARIES(cchat_server pthread)
