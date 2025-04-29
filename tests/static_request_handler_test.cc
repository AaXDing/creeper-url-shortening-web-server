#include "static_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class StaticRequestHandlerTest : public StaticRequestHandler {
  public:
  StaticRequestHandlerTest(std::string base_uri, std::string root_path)
      : StaticRequestHandler(base_uri, root_path) {}

  std::string handle_request(Request& req, Response& res) const {
    return StaticRequestHandler::handle_request(req, res);
  }

  std::string response_to_string(const Response& res) const {
    return StaticRequestHandler::response_to_string(res);
  }

  std::string get_file_content_type(const std::string& file_path) const {
    return StaticRequestHandler::get_file_content_type(file_path);
  }

  std::string generate_file_path(const std::string& file_path) const {
    return StaticRequestHandler::generate_file_path(file_path);
  }
};

class StaticRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::string base_uri = "/static";
  std::string root_path = "/";
  StaticRequestHandlerTest handler = StaticRequestHandlerTest(base_uri, root_path);
  Request request;
  Response response;
  std::string response_str;
};

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test1/test.txt";

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
  request.uri = "/static/test1/test.txt///";

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

TEST_F(StaticRequestHandlerTestFixture, PdfFileRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test2/creeper.pdf";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "application/pdf");
}

TEST_F(StaticRequestHandlerTestFixture, ZipFileRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test1/creeper.zip";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "application/zip");
}

TEST_F(StaticRequestHandlerTestFixture, JPEGFileRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test1/creeper.jpg";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "image/jpeg");
}

TEST_F(StaticRequestHandlerTestFixture, HTMLFileRequest) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test2/test.html";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "text/html");
}

TEST_F(StaticRequestHandlerTestFixture, FileNotSupported) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test2/file_not_supported";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response.status_code, 200);
  EXPECT_EQ(response.status_message, "OK");
  EXPECT_EQ(response.version, "HTTP/1.1");
  EXPECT_EQ(response.content_type, "application/octet-stream");
}

TEST_F(StaticRequestHandlerTestFixture, InvalidRequestOnFolder) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test2/empty";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response_str,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found");
}

TEST_F(StaticRequestHandlerTestFixture, FileDoesNotExist) {
  request.valid = true;
  request.version = "HTTP/1.1";
  request.method = "GET";
  request.uri = "/static/test1/invalid_file.txt";

  response_str = handler.handle_request(request, response);

  EXPECT_EQ(response_str,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "404 Not Found");
}

TEST_F(StaticRequestHandlerTestFixture, FilePath) {
  std::string file_path = handler.generate_file_path("/static/test1/test.txt");
  EXPECT_EQ(file_path, "../data/test1/test.txt");
}

TEST_F(StaticRequestHandlerTestFixture, FilePathWithTrailingSlash) {
  std::string file_path = handler.generate_file_path("/static/test1/test.txt//");
  EXPECT_EQ(file_path, "../data/test1/test.txt");
}