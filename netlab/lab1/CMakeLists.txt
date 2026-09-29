cmake_minimum_required(VERSION 3.22)

project(2026-lab1-myFTP)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

find_package(Threads REQUIRED)

add_executable(ftp_server ftp_server.cpp server_replies.cpp)
target_link_libraries(ftp_server PRIVATE Threads::Threads)

add_executable(ftp_client ftp_client.cpp client_requests.cpp)
