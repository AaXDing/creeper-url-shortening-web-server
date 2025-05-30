#include "http_header.h"

#include "gtest/gtest.h"

class HttpHeaderTestFixture : public ::testing::Test {
 protected:
  Request req;
  std::string request_str;
  Response res;
  std::string response_str;
};

TEST_F(HttpHeaderTestFixture, RequestToString) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/echo";
  req.headers.push_back({"Host", "www.example.com"});
  req.headers.push_back({"User-Agent", "curl/7.64.1"});
  req.headers.push_back({"Accept", "*/*"});

  request_str = req.to_string();

  EXPECT_EQ(request_str,
            "GET /echo HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n"
            "\r\n");
}

TEST_F(HttpHeaderTestFixture, ResponseToString) {
  res.status_code = 200;
  res.status_message = "OK";
  res.version = "HTTP/1.1";
  res.headers.push_back({"Content-Type", "text/plain"});
  res.body = "Hello, World!";
  response_str = res.to_string();

  EXPECT_EQ(response_str,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, World!");
}

TEST_F(HttpHeaderTestFixture, ResponseToStringEmptyBody) {
  res.status_code = 204;
  res.status_message = "No Content";
  res.version = "HTTP/1.1";
  res.headers.push_back({"Content-Type", "text/plain"});
  res.body = "";
  response_str = res.to_string();

  EXPECT_EQ(response_str,
            "HTTP/1.1 204 No Content\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 0\r\n"
            "\r\n");
}

TEST_F(HttpHeaderTestFixture, RepsonseConstructor) {
  res = Response("HTTP/1.1", 200, "OK", {{"Content-Type", "text/plain"}},
                 "Hello, World!");
  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "Hello, World!");
}