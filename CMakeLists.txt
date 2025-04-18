cmake_minimum_required(VERSION 3.10.0)
project(creeper)

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

# add main executable
add_executable(server src/server_main.cc)
target_link_libraries(server server_lib session_lib request_parser_lib config_parser_lib Boost::system)

add_executable(request_parser_lib_test tests/request_parser_test.cc)
target_link_libraries(request_parser_lib_test request_parser_lib gtest_main)

add_executable(config_parser_lib_test tests/config_parser_test.cc)
target_link_libraries(config_parser_lib_test config_parser_lib gtest_main)

# add_executable(server_lib_test tests/server_test.cc)
# target_link_libraries(server_lib_test server_lib session_lib gtest_main)

#add_executable(echo_response_test tests/echo_response_test.cc)
#target_link_libraries(echo_response_test echo_response_lib gtest_main)

gtest_discover_tests(request_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests PROPERTIES LABELS unit)   
gtest_discover_tests(config_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests PROPERTIES LABELS unit)   
#gtest_discover_tests(echo_response_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests PROPERTIES LABELS unit)   
# gtest_discover_tests(server_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests PROPERTIES LABELS unit)


include(cmake/CodeCoverageReportConfig.cmake)

# ---------- UNIT‑TEST COVERAGE ----------
generate_coverage_report(
    NAME         unit_coverage              # custom target:  unit_coverage
    LABEL        unit       
    TARGETS      server_lib
                 session_lib
                 request_parser_lib
                 config_parser_lib
                 #echo_response_lib
                 server
    TESTS        request_parser_lib_test
                 config_parser_lib_test
                 #echo_response_test
)

# ---------- INTEGRATION‑TEST COVERAGE (blank for now) ----------
generate_coverage_report(
    NAME         integration_coverage       # custom target: integration_coverage
    LABEL        integration
    TARGETS      server_lib
                 session_lib
                 request_parser_lib
                 config_parser_lib
                 #echo_response_lib
                 server
    # TESTS left empty until you add integration tests
)

# After the two generate_coverage_report() calls
# ----------------------------------------------
add_custom_target(coverage
    DEPENDS unit_coverage integration_coverage
    COMMENT "Build both unit and integration test coverage reports")
