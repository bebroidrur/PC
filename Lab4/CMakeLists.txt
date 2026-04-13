cmake_minimum_required(VERSION 3.20)
project(Lab4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Lab4Server
        server_main.cpp
        server.cpp
)

add_executable(Lab4Client
        client_main.cpp
        client.cpp
)