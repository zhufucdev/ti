cmake_minimum_required(VERSION 3.24)
project(TiProtocol)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(TiProtocol "${SOURCES}")
target_link_libraries(TiProtocol SQLite3)
target_include_directories(TiProtocol PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
