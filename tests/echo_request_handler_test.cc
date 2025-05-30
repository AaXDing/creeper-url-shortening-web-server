#include "echo_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class EchoRequestHandlerTest : public EchoRequestHandler {
 public:
  EchoRequestHandlerTest(const std::string& a,
                         const std::shared_ptr<EchoRequestHandlerArgs>& b)
      : EchoRequestHandler(a, b) {}

  Response call_handle_request(Request& req) {
    std::unique_ptr<Response> p = handle_request(req);
    return *p;
  }
};

class EchoRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::shared_ptr<EchoRequestHandlerTest> handler =
      std::make_shared<EchoRequestHandlerTest>(
          "", std::make_shared<EchoRequestHandlerArgs>());
  std::shared_ptr<EchoRequestHandlerArgs> args;
  Request req;
  Response res;
  std::string request_str;
  NginxConfigParser parser = NginxConfigParser();
  NginxConfig config;
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
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
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
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "400 Bad Request");
}

TEST_F(EchoRequestHandlerTestFixture, ValidEchoRequestWithArgs) {
  bool success =
      parser.parse("request_handler_testcases/valid_echo_config", &config);
  EXPECT_TRUE(success);
  args = EchoRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_NE(args, nullptr);
}

TEST_F(EchoRequestHandlerTestFixture, InvalidEchoRequestWithArgs) {
  bool success =
      parser.parse("request_handler_testcases/invalid_echo_config", &config);
  EXPECT_TRUE(success);
  args = EchoRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(EchoRequestHandlerTestFixture, GetType) {
  EXPECT_EQ(handler->get_type(),
            RequestHandler::HandlerType::ECHO_REQUEST_HANDLER);
}