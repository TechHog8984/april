cmake_minimum_required(VERSION 3.16)
project(april LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(april,
    src)

add_executable(april
    src/classreader.cpp
    src/codegenluau.cpp
    src/main.cpp)
