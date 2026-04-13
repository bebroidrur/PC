cmake_minimum_required(VERSION 3.20)
project(Lab4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Lab4
        main.cpp
        server.cpp
        client.cpp
)