// echo_response_test.cc
// Code below modified based on LLM outputs
#include "gtest/gtest.h"
#include "session.h"
#include <string>

class SessionTest : public session {

public:
    explicit SessionTest(boost::asio::io_service &io_service)
        : session(io_service) {}

    // Expose the private handle_echo_response method for testing.
    std::string call_handle_echo_response(size_t bytes_transferred) {
        return session::handle_echo_response(bytes_transferred);
    }

    // write to data_ buffer for testing
    void set_data(const std::string &data) {
        std::copy(data.begin(), data.end(), data_);
        data_[data.size()] = '\0'; // null-terminate the string
    }

};

// Define a test fixture for session tests.
class SessionTestFixture : public ::testing::Test {
protected:
    std::string input;
    std::string http_version = "HTTP/1.1";
    boost::asio::io_service io_service;
    std::shared_ptr<SessionTest> sess = std::make_shared<SessionTest>(io_service);
};

// Test that a valid request produces a 200 OK response with echoed body.
// Assumes the request parser is well-tested and valid.
TEST_F(SessionTestFixture, ValidRequestReturns200) {
    input = "GET /index.html HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n";
    sess->set_data(input);

    std::string expected_response =
        http_version + " 200 OK\r\n" +
        "Content-Type: text/plain\r\n" +
        "Content-Length: " + std::to_string(input.size()) + "\r\n" +
        "\r\n" +
        input;

    std::string response = sess->call_handle_echo_response(input.size());
    EXPECT_EQ(response, expected_response);
}

//Test that an invalid request produces a 400 Bad Request response.
// Assumes the request parser is well-tested and valid.
TEST_F(SessionTestFixture, InvalidRequestReturns400) {
    input = " /weird HTTP/1.1\r\n"
    "Host: weird.com\r\n"
    "\r\n";
    sess->set_data(input);

    std::string body = "400 Bad Request"; // Define the body for invalid requests.
    std::string expected_response =
        http_version + " 400 Bad Request\r\n" +
        "Content-Type: text/plain\r\n" +
        "Content-Length: " + std::to_string(body.size()) + "\r\n" +
        "\r\n" +
        body;

    std::string response = sess->call_handle_echo_response(input.size());
    EXPECT_EQ(response, expected_response);
}
