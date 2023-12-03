cmake_minimum_required(VERSION 3.24)
project(Sha3)

add_library(Sha3 "${CMAKE_CURRENT_SOURCE_DIR}/src/sha3.cpp")
target_include_directories(Sha3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")