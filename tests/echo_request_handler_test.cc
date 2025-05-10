#include "echo_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class EchoRequestHandlerTest : public EchoRequestHandler {
 public:
  EchoRequestHandlerTest(const std::string& a, const std::string& b) : EchoRequestHandler(a, b) {}

  Response call_handle_request(Request& req) {
    std::unique_ptr<Response> p = handle_request(req);
    return *p;
  }
};

class EchoRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::shared_ptr<EchoRequestHandlerTest> handler =
      std::make_shared<EchoRequestHandlerTest>("", "");
  Request req;
  Response res;
  std::string request_str;
};

TEST_F(EchoRequestHandlerTestFixture, ValidEchoRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  res = handler->call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body,
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

  res = handler->call_handle_request(req);

  EXPECT_EQ(res.status_code, 400);
  EXPECT_EQ(res.status_message, "Bad Request");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body, "400 Bad Request");
}