cmake_minimum_required(VERSION 3.24)
project(SQLite3)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
add_library(SQLite3 "${SOURCES}")
target_include_directories(SQLite3 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
