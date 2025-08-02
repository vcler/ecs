cmake_minimum_required(VERSION 3.28.1)

project(colony)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23 REQUIRED)
set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -pedantic")

add_executable(colony
    main.cpp
)

target_include_directories(colony PUBLIC ${CMAKE_SOURCE_DIR})

