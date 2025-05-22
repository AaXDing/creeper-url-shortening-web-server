#include "not_found_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class NotFoundRequestHandlerTest : public NotFoundRequestHandler {
 public:
  NotFoundRequestHandlerTest(
      const std::string& a,
      const std::shared_ptr<NotFoundRequestHandlerArgs>& b)
      : NotFoundRequestHandler(a, b) {}

  Response call_handle_request(Request& req) {
    std::unique_ptr<Response> p = handle_request(req);
    return *p;
  }
};

class NotFoundRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::shared_ptr<NotFoundRequestHandlerTest> handler =
      std::make_shared<NotFoundRequestHandlerTest>(
          "", std::make_shared<NotFoundRequestHandlerArgs>());
  std::shared_ptr<NotFoundRequestHandlerArgs> args;
  Request req;
  Response res;
  NginxConfigParser parser = NginxConfigParser();
  NginxConfig config;
};

TEST_F(NotFoundRequestHandlerTestFixture, ValidRequestNotFound) {
  // Test with a valid request that should result in 404
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/nonexistent";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});

  res = handler->call_handle_request(req);

  EXPECT_EQ(res.status_code, 404);
  EXPECT_EQ(res.status_message, "Not Found");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body, "404 Not Found");
}

TEST_F(NotFoundRequestHandlerTestFixture, InvalidRequestNotFound) {
  // Test with an invalid request that should still result in 404
  req.valid = false;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/nonexistent";
  req.headers.push_back({"Host", "www.example.com"});

  res = handler->call_handle_request(req);

  EXPECT_EQ(res.status_code, 404);
  EXPECT_EQ(res.status_message, "Not Found");
  EXPECT_EQ(res.version,
            HTTP_VERSION);  // Should use default version for invalid requests
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body, "404 Not Found");
}

TEST_F(NotFoundRequestHandlerTestFixture, ValidRequestNotFoundArgs) {
  bool success =
      parser.parse("request_handler_testcases/valid_not_found_config", &config);
  EXPECT_TRUE(success);
  args = NotFoundRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_NE(args, nullptr);
}

TEST_F(NotFoundRequestHandlerTestFixture, InvalidRequestNotFoundArgs) {
  bool success = parser.parse(
      "request_handler_testcases/invalid_not_found_config", &config);
  EXPECT_TRUE(success);
  args = NotFoundRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(NotFoundRequestHandlerTestFixture, GetType) {
  // Test the get_type method
  EXPECT_TRUE(handler->get_type() ==
              RequestHandler::HandlerType::NOT_FOUND_REQUEST_HANDLER);
}