cmake_minimum_required(VERSION 3.24)
include(FetchContent)
project(ti)
cmake_policy(VERSION 3.24)

FetchContent_Declare(
        googletest
        # Specify the commit you depend on and update it regularly.
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
FetchContent_MakeAvailable(googletest)

if (WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif ()
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
set(THIRDPARTY_LIB "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
add_subdirectory("${THIRDPARTY_LIB}/NanoId")
add_subdirectory("${THIRDPARTY_LIB}/Aes256")
add_subdirectory("${THIRDPARTY_LIB}/Argon2")

set(MODULES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/module")
add_subdirectory("${MODULES_DIR}/protocol")
add_subdirectory("${MODULES_DIR}/server")
add_subdirectory("${MODULES_DIR}/client")

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
add_executable(ti_server "${SRC_DIR}/server/main.cpp")
add_executable(ti_client "${SRC_DIR}/client/main.cpp")
target_link_libraries(ti_server TiServer)
target_link_libraries(ti_client TiClient)

enable_testing()
file(GLOB TEST_SOURCES "${SRC_DIR}/test/*.cc")
add_executable(ti_test ${TEST_SOURCES})
target_link_libraries(ti_test GTest::gtest_main TiServer TiClient NanoId)

include(GoogleTest)
gtest_discover_tests(ti_test)

add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)
