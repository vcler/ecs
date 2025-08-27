add_executable(test
    test.cpp)

target_include_directories(test PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR})

