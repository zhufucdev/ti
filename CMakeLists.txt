cmake_minimum_required(VERSION 3.13)
project(ti)
set(CMAKE_CXX_STANDARD 14)

file(GLOB CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp")
file(GLOB SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/server/*.cpp")
add_executable(ti_client ${CLIENT_SOURCES})
add_executable(ti_server ${SERVER_SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") 

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
