#include "static_request_handler.h"

#include "gtest/gtest.h"
#include "http_header.h"

class StaticRequestHandlerTestFixture : public ::testing::Test {
 protected:
  StaticRequestHandler handler;
  Request request;
  Response response;
};

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/test.txt";

  handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/plain");
  EXPECT_EQ(response.body, "line1\nline2\n\nline4");
}

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequestWithExtraSlashes) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/test.txt///";

  handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/plain");
  EXPECT_EQ(response.body, "line1\nline2\n\nline4");
}

TEST_F(StaticRequestHandlerTestFixture, FileDoesNotExist) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/example/invalid_file.txt";

  handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 404);
  EXPECT_EQ(response.status_message, "Not Found");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/plain");
  EXPECT_EQ(response.body, "404 Not Found");
}