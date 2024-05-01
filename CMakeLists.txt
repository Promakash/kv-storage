cmake_minimum_required(VERSION 3.13)

project(postupashki-course)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(cmake/Utils.cmake)

include_catch2()

include_directories(utils)

add_subdirectory(tasks)
