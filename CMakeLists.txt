cmake_minimum_required(VERSION 3.13)
project(ti)
set(CMAKE_CXX_STANDARD 14)

set(APP_SRC src/main.cpp)

add_executable(ti ${APP_SRC})
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include") 

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
