#include "echo_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class EchoRequestHandlerTest : public EchoRequestHandler {
 public:
  std::string call_handle_request(Request& req, Response& res) const {
    return handle_request(req, res);
  }

  // Expose the private methods for testing
  // Testing regular response_to_string method in request_handler.cc
  std::string call_response_to_string(const Response& res) const {
    return response_to_string(res);
  }

  std::string call_request_to_string(const Request& req) const {
    return request_to_string(req);
  }
};

class EchoRequestHandlerTestFixture : public ::testing::Test {
 protected:
  EchoRequestHandlerTest handler;
  Request req;
  Response res;
  std::string response_str;
};

TEST_F(EchoRequestHandlerTestFixture, ValidEchoRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  response_str = handler.call_handle_request(req, res);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body,
            "GET /echo HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n\r\n");
  EXPECT_EQ(response_str,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 83\r\n"
            "\r\n"
            "GET /echo HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n\r\n");
}

TEST_F(EchoRequestHandlerTestFixture, InvalidEchoRequest) {
  req.valid = false;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  response_str = handler.call_handle_request(req, res);

  EXPECT_EQ(response_str,
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "\r\n"
            "400 Bad Request");
}

TEST_F(EchoRequestHandlerTestFixture, ResponseToString) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  handler.call_handle_request(req, res);
  std::string response_str = handler.call_response_to_string(res);

  EXPECT_EQ(response_str,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 83\r\n"
            "\r\n"
            "GET /echo HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n\r\n");
}

TEST_F(EchoRequestHandlerTestFixture, RequestToString) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  std::string request_str = handler.call_request_to_string(req);

  EXPECT_EQ(request_str,
            "GET /echo HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n"
            "\r\n");
}