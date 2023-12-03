cmake_minimum_required(VERSION 3.24)
project(TiClient)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(TiClient "${SOURCES}")
target_link_libraries(TiClient PRIVATE TiProtocol)
target_include_directories(TiClient PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")