cmake_minimum_required(VERSION 3.8)
project(Starry)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES connection.cpp main.cpp connection.h)
add_executable(Starry ${SOURCE_FILES})