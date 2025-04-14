cmake_minimum_required(VERSION 3.16.0)

add_executable(test simulation.cpp)

target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/../src)