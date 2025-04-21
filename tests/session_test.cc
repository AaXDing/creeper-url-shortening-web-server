// echo_response_test.cc
// Code below modified based on LLM outputs
#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "session.h"
#include <boost/asio.hpp>
#include <string>

using ::testing::AtLeast;

class SessionTest : public session {

public:
    explicit SessionTest(boost::asio::io_service &io_service)
        : session(io_service), is_deleted(nullptr) {}

        ~SessionTest() {
            if (is_deleted != nullptr) {
                *is_deleted = true; // Set the flag to true when deleted
            }
        }
    // Expose the private handle_echo_response method for testing.
    std::string call_handle_echo_response(size_t bytes_transferred) {
        return session::handle_echo_response(bytes_transferred);
    }

    void call_handle_read(const boost::system::error_code &error,
                  size_t bytes_transferred) {
        session::handle_read(error, bytes_transferred);
    }
    
    void call_handle_write(const boost::system::error_code &error) {
        session::handle_write(error);
    }

    // write to data_ buffer for testing
    void set_data(const std::string &data) {
        std::copy(data.begin(), data.end(), data_);
        data_[data.size()] = '\0'; // null-terminate the string
    }
    
    // This is a flag to check if the session was deleted
    // If there is an error in the session, it will delete itself
    // so we can't track if it was deleted or not. The pointer will 
    // be the same, but the memory is freed. This is a workaround for that.
    bool* is_deleted; 

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

TEST_F(SessionTestFixture, HandleReadSuccessProcessesData) {
    input = "GET /index.html HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n";
    sess->set_data(input);

    bool deleted = false;
    sess->is_deleted = &deleted; // Set the deletion flag

    boost::system::error_code success_ec;
    sess->call_handle_read(success_ec, input.size());
    
    EXPECT_EQ(success_ec.value(), 0); // No error
    EXPECT_FALSE(deleted); // Session should still exist
}

TEST_F(SessionTestFixture, HandleReadErrorDeletesSession) {
    SessionTest* raw_sess = new SessionTest(io_service);
    boost::system::error_code error_ec(boost::asio::error::eof);

    bool deleted = false;
    raw_sess->is_deleted = &deleted; // Set the deletion flag

    raw_sess->call_handle_read(error_ec, 0);
    
    EXPECT_NE(error_ec.value(), 0); // Error occurred
    EXPECT_TRUE(deleted); // Check if the deletion flag was set
}

TEST_F(SessionTestFixture, HandleWriteSuccessProcessesData) {
    boost::system::error_code success_ec;
    sess->call_handle_write(success_ec);

    bool deleted = false;
    sess->is_deleted = &deleted; // Set the deletion flag
    
    EXPECT_EQ(success_ec.value(), 0); // No error
    EXPECT_FALSE(deleted); // Session should still exist
}

TEST_F(SessionTestFixture, HandleWriteErrorDeletesSession) {
    SessionTest* raw_sess = new SessionTest(io_service);
    boost::system::error_code error_ec(boost::asio::error::eof);

    bool deleted = false;
    raw_sess->is_deleted = &deleted; // Set the deletion flag

    raw_sess->call_handle_write(error_ec);
    
    EXPECT_NE(error_ec.value(), 0); // No error
    EXPECT_TRUE(deleted); // Check if the deletion flag was set
}