cmake_minimum_required(VERSION 3.28.1)

project(ecs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23 REQUIRED)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic")

add_subdirectory(test)

