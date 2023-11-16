cmake_minimum_required(VERSION 3.13)
project(Argon2)

file(GLOB SRC "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*")
add_library(Argon2 "${SRC}")
target_include_directories(Argon2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")