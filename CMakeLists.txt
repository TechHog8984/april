cmake_minimum_required(VERSION 3.16)
project(april LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZIP REQUIRED libzip)

add_executable(april
    src/jarloader.cpp
    src/classreader.cpp
    src/codegenluau.cpp
    src/main.cpp)

include_directories(april,
    src
    ${LIBZIP_INCLUDE_DIRS})

target_link_directories(april PRIVATE
    ${LIBZIP_LIBRARY_DIRS}
)

target_link_libraries(april PRIVATE
    ${LIBZIP_LIBRARIES}
)
