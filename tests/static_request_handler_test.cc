#include "static_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class StaticRequestHandlerTestFixture : public ::testing::Test {
 protected:
  StaticRequestHandler handler;
  Request request;
  Response response;
  std::string response_str;
};

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/test.txt";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/plain");
  EXPECT_EQ(response.body, "line1\nline2\n\nline4");
  EXPECT_EQ(response_str,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 18\r\n"
            "\r\n"
            "line1\nline2\n\nline4");
}

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequestWithExtraSlashes) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/test.txt///";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/plain");
  EXPECT_EQ(response.body, "line1\nline2\n\nline4");
  EXPECT_EQ(response_str,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 18\r\n"
            "\r\n"
            "line1\nline2\n\nline4");
}

TEST_F(StaticRequestHandlerTestFixture, FileDoesNotExist) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/invalid_file.txt";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response_str,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found");
}