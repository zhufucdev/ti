cmake_minimum_required(VERSION 3.24)
project(TiServer)

file(GLOB SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
add_library(TiServer "${SOURCES}")
target_link_libraries(TiServer PUBLIC TiProtocol PRIVATE NanoId)
target_include_directories(TiServer PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")