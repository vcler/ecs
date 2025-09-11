cmake_minimum_required(VERSION 3.28.1)

project(colony)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23 REQUIRED)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic")

add_subdirectory(test)

add_executable(ecs
    main.cpp
)

target_include_directories(ecs PUBLIC ${CMAKE_SOURCE_DIR})

