// echo_response_test.cc
// Code below modified based on LLM outputs
#include "gtest/gtest.h"
#include "echo_response.h"
#include <string>

// Define a test fixture for echo_response tests.
// Note: The make_echo_response function does not judge the validity of the request;
// it solely relies on the 'valid' parameter passed in. Therefore, using an empty
// request is sufficient to simulate an invalid request.
class EchoResponseTestFixture : public ::testing::Test {
protected:
    std::string valid_request;
    std::string invalid_request;
    std::string http_version;

    // Set up the test inputs.
    void SetUp() override {
        valid_request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
        invalid_request = "";  // Example of an invalid request
        http_version = "HTTP/1.1";
    }
};

// Test that a valid request produces a 200 OK response with echoed body.
TEST_F(EchoResponseTestFixture, ValidRequestReturns200) {
    std::string expected_response =
        http_version + " 200 OK\r\n" +
        "Content-Type: text/plain\r\n" +
        "Content-Length: " + std::to_string(valid_request.size()) + "\r\n" +
        "\r\n" +
        valid_request;

    std::string response = make_echo_response(http_version, valid_request, true);
    EXPECT_EQ(response, expected_response);
}

// Test that an invalid request produces a 400 Bad Request response.
TEST_F(EchoResponseTestFixture, InvalidRequestReturns400) {
    std::string body = "400 Bad Request";
    std::string expected_response =
        http_version + " 400 Bad Request\r\n" +
        "Content-Type: text/plain\r\n" +
        "Content-Length: " + std::to_string(body.size()) + "\r\n" +
        "\r\n" +
        body;

    std::string response = make_echo_response(http_version, invalid_request, false);
    EXPECT_EQ(response, expected_response);
}
