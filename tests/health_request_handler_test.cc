#include "health_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class HealthRequestHandlerTest : public HealthRequestHandler {
 public:
  HealthRequestHandlerTest(const std::string& a, const std::shared_ptr<HealthRequestHandlerArgs>& b)
      : HealthRequestHandler(a, b) {}

  Response call_handle_request(Request& req) {
    std::unique_ptr<Response> p = handle_request(req);
    return *p;
  }
};

class HealthRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::shared_ptr<HealthRequestHandlerTest> handler =
      std::make_shared<HealthRequestHandlerTest>("", std::make_shared<HealthRequestHandlerArgs>());
  Request req;
  Response res;
  std::string request_str;
  NginxConfigParser parser = NginxConfigParser();
  NginxConfig config;
};

TEST_F(HealthRequestHandlerTestFixture, ValidHealthRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/health";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  res = handler->call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.content_type, "text/plain");
  EXPECT_EQ(res.body, "OK");
}