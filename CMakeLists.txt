cmake_minimum_required(VERSION 3.13)
project(ti)
find_package(SQLite3 REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(THIRDPARTY_LIB "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
add_subdirectory("${THIRDPARTY_LIB}/NanoId")
add_subdirectory("${THIRDPARTY_LIB}/Aes256")
add_subdirectory("${THIRDPARTY_LIB}/Argon2")

file(GLOB CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp")
file(GLOB SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/server/*.cpp")
file(GLOB SHARED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*")

add_executable(ti_client ${CLIENT_SOURCES} ${SHARED_SOURCES})
add_executable(ti_server ${SERVER_SOURCES} ${SHARED_SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(ti_server NanoId SQLite3)
target_link_libraries(ti_client NanoId SQLite3)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
