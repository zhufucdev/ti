cmake_minimum_required(VERSION 3.13)
project(NanoId)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(NanoId "${SRC}")
target_include_directories(NanoId PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
