cmake_minimum_required(VERSION 3.10.0)
project(creeper)

# Enable Python for integration test
find_package(Python3 COMPONENTS Interpreter REQUIRED)

# Make sure developers do not run cmake in the main project directory, to keep
# build artifacts from becoming clutter
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds not allowed.
    Please make a new directory (called a build directory) and run CMake from there.
    You may need to remove CMakeCache.txt." )
endif()

# Turn on debug builds if we are building in a devel shell
if (CMAKE_BUILD_TYPE STREQUAL "" AND "$ENV{DEVEL_SHELL}" STREQUAL "1")
    message(STATUS "Setting debug build type by default in devel shell")
    set(CMAKE_BUILD_TYPE Debug)
endif()

# Output binaries to a sub directory "bin"
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Enable cmake testing
include(CTest)
enable_testing()

# Enable GoogleTest
include(GoogleTest)
add_subdirectory(/usr/src/googletest googletest)

# Enable Boost
# Use static libraries so binaries can be deployed without a full boost install
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.50 REQUIRED COMPONENTS system)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# add libraries
add_library(server_lib src/server.cc)
add_library(session_lib src/session.cc)
add_library(request_parser_lib src/request_parser.cc)
add_library(config_parser_lib src/config_parser.cc)
add_library(request_handler_echo_lib src/request_handler_echo.cc)

# add main executable
add_executable(server src/server_main.cc)
target_link_libraries(server server_lib session_lib request_handler_echo_lib request_parser_lib config_parser_lib Boost::system)

add_executable(request_parser_lib_test tests/request_parser_test.cc)
target_link_libraries(request_parser_lib_test request_parser_lib gtest_main)

add_executable(request_handler_echo_lib_test tests/request_handler_echo_test.cc)
target_link_libraries(request_handler_echo_lib_test request_handler_echo_lib gtest_main)

add_executable(config_parser_lib_test tests/config_parser_test.cc)
target_link_libraries(config_parser_lib_test config_parser_lib gtest_main)

add_executable(server_lib_test tests/server_test.cc)
target_link_libraries(server_lib_test server_lib session_lib request_handler_echo_lib request_parser_lib gtest_main gmock pthread)

add_executable(session_lib_test tests/session_test.cc)
target_link_libraries(session_lib_test session_lib request_handler_echo_lib request_parser_lib gtest_main gmock pthread)

gtest_discover_tests(request_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(config_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_lib_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
gtest_discover_tests(server_lib_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


# --- Coverage support for unit tests only ---
include(cmake/CodeCoverageReportConfig.cmake)

generate_coverage_report(
    TARGETS
        server_lib
        session_lib
        request_parser_lib
        request_handler_echo_lib
        config_parser_lib
        server
    TESTS
        request_parser_lib_test
        request_handler_echo_lib_test
        config_parser_lib_test
        server_lib_test
        session_lib_test
)

# Integration test using Python script
add_test(
    NAME IntegrationTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.py 
            ${CMAKE_CURRENT_SOURCE_DIR}/my_config
)
# If only wants to call integration
set_tests_properties(IntegrationTest PROPERTIES LABELS integration)

