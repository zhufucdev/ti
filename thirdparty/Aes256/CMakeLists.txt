cmake_minimum_required(VERSION 3.13)
project(Aes256)

add_library(Aes256 "${CMAKE_CURRENT_SOURCE_DIR}/src/aes256.cpp")
target_include_directories(Aes256 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")