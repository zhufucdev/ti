cmake_minimum_required(VERSION 3.13)
project(ti)
find_package(SQLite3 REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

file(GLOB CLIENT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/client/*.cpp")
file(GLOB SERVER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/server/*.cpp")
file(GLOB SHARED_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c*" "${CMAKE_CURRENT_SOURCE_DIR}/src/nanoid/*.cpp")

add_executable(ti_client ${CLIENT_SOURCES} ${SHARED_SOURCES})
add_executable(ti_server ${SERVER_SOURCES} ${SHARED_SOURCES})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories(${SQLITE3_INCLUDE_DIRS})
target_link_libraries(ti_server SQLite3)
target_link_libraries(ti_client SQLite3)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
