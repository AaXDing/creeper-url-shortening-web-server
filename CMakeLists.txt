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
find_package(Boost 1.50 REQUIRED
  COMPONENTS
    system
    filesystem
    log
    log_setup
    json
)
message(STATUS "Boost version: ${Boost_VERSION}")

include_directories(include)

# add libraries
add_library(server_lib src/server.cc)
target_link_libraries(server_lib PUBLIC Boost::system logging_lib)

add_library(session_lib src/session.cc)
add_library(request_parser_lib src/request_parser.cc)
target_link_libraries(request_parser_lib PUBLIC logging_lib)

add_library(config_parser_lib src/config_parser.cc)
target_link_libraries(config_parser_lib PUBLIC logging_lib)

add_library(http_header_lib src/http_header.cc)
target_link_libraries(http_header_lib PUBLIC logging_lib)

add_library(registry_lib src/registry.cc)
target_link_libraries(registry_lib PUBLIC logging_lib)

add_library(echo_request_handler_lib src/echo_request_handler.cc)
target_link_libraries(echo_request_handler_lib PUBLIC http_header_lib logging_lib registry_lib)

add_library(static_request_handler_lib src/static_request_handler.cc)
target_link_libraries(static_request_handler_lib PUBLIC http_header_lib Boost::filesystem logging_lib registry_lib)

add_library(not_found_request_handler_lib src/not_found_request_handler.cc)
target_link_libraries(not_found_request_handler_lib PUBLIC http_header_lib logging_lib registry_lib)

add_library(request_handler_dispatcher_lib src/request_handler_dispatcher.cc)
target_link_libraries(request_handler_dispatcher_lib PUBLIC http_header_lib config_parser_lib registry_lib logging_lib)

add_library(logging_lib src/logging.cc)
target_link_libraries(logging_lib PUBLIC Boost::log Boost::log_setup Boost::system Boost::filesystem)

add_library(file_entity_storage_lib src/file_entity_storage.cc)
target_link_libraries(file_entity_storage_lib logging_lib)

add_library(sim_entity_storage_lib src/sim_entity_storage.cc)
target_link_libraries(sim_entity_storage_lib logging_lib)

add_library(crud_request_handler_lib src/crud_request_handler.cc)
target_link_libraries(crud_request_handler_lib PUBLIC http_header_lib logging_lib registry_lib file_entity_storage_lib sim_entity_storage_lib Boost::filesystem Boost::json)

# add main executable
add_executable(server src/server_main.cc src/echo_request_handler.cc src/static_request_handler.cc src/not_found_request_handler.cc src/crud_request_handler.cc)
target_link_libraries(server server_lib session_lib http_header_lib request_parser_lib request_handler_dispatcher_lib config_parser_lib crud_request_handler_lib file_entity_storage_lib sim_entity_storage_lib Boost::system)

add_executable(request_parser_lib_test tests/request_parser_test.cc)
target_link_libraries(request_parser_lib_test http_header_lib request_parser_lib gtest_main)

add_executable(echo_request_handler_lib_test tests/echo_request_handler_test.cc)
target_link_libraries(echo_request_handler_lib_test http_header_lib echo_request_handler_lib config_parser_lib registry_lib logging_lib gtest_main)

add_executable(static_request_handler_lib_test tests/static_request_handler_test.cc)
target_link_libraries(static_request_handler_lib_test http_header_lib static_request_handler_lib config_parser_lib registry_lib logging_lib gtest_main)

add_executable(not_found_request_handler_lib_test tests/not_found_request_handler_test.cc)
target_link_libraries(not_found_request_handler_lib_test http_header_lib not_found_request_handler_lib config_parser_lib registry_lib logging_lib gtest_main)

add_executable(config_parser_lib_test tests/config_parser_test.cc src/echo_request_handler.cc src/static_request_handler.cc src/not_found_request_handler.cc src/crud_request_handler.cc)
target_link_libraries(config_parser_lib_test config_parser_lib registry_lib echo_request_handler_lib static_request_handler_lib not_found_request_handler_lib crud_request_handler_lib file_entity_storage_lib sim_entity_storage_lib gtest_main)

add_executable(server_lib_test tests/server_test.cc)
target_link_libraries(server_lib_test server_lib session_lib http_header_lib request_parser_lib request_handler_dispatcher_lib file_entity_storage_lib sim_entity_storage_lib gtest_main gmock pthread)

add_executable(session_lib_test tests/session_test.cc src/echo_request_handler.cc src/static_request_handler.cc src/not_found_request_handler.cc src/crud_request_handler.cc)
target_link_libraries(session_lib_test session_lib http_header_lib request_parser_lib request_handler_dispatcher_lib crud_request_handler_lib config_parser_lib file_entity_storage_lib sim_entity_storage_lib gtest_main gmock pthread)

add_executable(logging_lib_test tests/logging_test.cc)
target_link_libraries(logging_lib_test logging_lib gtest_main)

add_executable(request_handler_dispatcher_lib_test tests/request_handler_dispatcher_test.cc src/echo_request_handler.cc src/static_request_handler.cc src/not_found_request_handler.cc src/crud_request_handler.cc)
target_link_libraries(request_handler_dispatcher_lib_test request_handler_dispatcher_lib http_header_lib request_parser_lib crud_request_handler_lib file_entity_storage_lib sim_entity_storage_lib config_parser_lib gtest_main)

add_executable(http_header_test tests/http_header_test.cc)
target_link_libraries(http_header_test http_header_lib gtest_main)

add_executable(registry_lib_test tests/registry_test.cc src/echo_request_handler.cc)
target_link_libraries(registry_lib_test registry_lib echo_request_handler_lib gtest_main)

add_executable(crud_request_handler_lib_test tests/crud_request_handler_test.cc)
target_link_libraries(crud_request_handler_lib_test crud_request_handler_lib http_header_lib config_parser_lib registry_lib logging_lib gtest_main)

add_executable(file_entity_storage_test tests/file_entity_storage_test.cc)
target_link_libraries(file_entity_storage_test
    file_entity_storage_lib
    gtest_main
    Boost::log_setup
    Boost::log
)

add_executable(sim_entity_storage_test tests/sim_entity_storage_test.cc)
target_link_libraries(sim_entity_storage_test
    sim_entity_storage_lib
    gtest_main
    Boost::log_setup
    Boost::log
)

gtest_discover_tests(http_header_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(request_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(config_parser_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(logging_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(session_lib_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
gtest_discover_tests(server_lib_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
gtest_discover_tests(echo_request_handler_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(static_request_handler_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(not_found_request_handler_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(request_handler_dispatcher_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(registry_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(crud_request_handler_lib_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(file_entity_storage_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
gtest_discover_tests(sim_entity_storage_test WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)

# --- Coverage support for unit tests only ---
include(cmake/CodeCoverageReportConfig.cmake)

generate_coverage_report(
    TARGETS
        server_lib
        session_lib
        request_parser_lib
        http_header_lib
        echo_request_handler_lib
        static_request_handler_lib
        not_found_request_handler_lib
        crud_request_handler_lib
        config_parser_lib
        logging_lib
        request_handler_dispatcher_lib
        registry_lib
        server
    TESTS
        http_header_test
        request_parser_lib_test
        echo_request_handler_lib_test
        static_request_handler_lib_test
        config_parser_lib_test
        not_found_request_handler_lib_test
        crud_request_handler_lib_test
        server_lib_test
        session_lib_test
        logging_lib_test
        request_handler_dispatcher_lib_test
        registry_lib_test
        file_entity_storage_test
)

# Integration test using Python script
add_test(
    NAME IntegrationTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${Python3_EXECUTABLE} 
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/integration_test.py 
            ${CMAKE_CURRENT_SOURCE_DIR}/my_config
)
# If only wants to call integration
set_tests_properties(IntegrationTest PROPERTIES LABELS integration)

