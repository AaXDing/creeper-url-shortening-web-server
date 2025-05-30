#include "static_request_handler.h"

#include <string>

#include "gtest/gtest.h"
#include "http_header.h"

class StaticRequestHandlerTest : public StaticRequestHandler {
 public:
  StaticRequestHandlerTest(std::string base_uri, std::string root_path)
      : StaticRequestHandler(
            std::move(base_uri),
            std::make_shared<StaticRequestHandlerArgs>(root_path)) {}

  Response call_handle_request(Request& req) {
    std::unique_ptr<Response> p = handle_request(req);
    return *p;
  }

  std::string call_get_file_content_type(const std::string& file_path) const {
    return StaticRequestHandler::get_file_content_type(file_path);
  }

  std::string call_generate_file_path(const std::string& file_path) const {
    return StaticRequestHandler::generate_file_path(file_path);
  }
};

class StaticRequestHandlerTestFixture : public ::testing::Test {
 protected:
  std::string base_uri = "/static";
  std::string root_path = "../data";
  std::unique_ptr<StaticRequestHandler> handler;
  std::shared_ptr<StaticRequestHandlerArgs> args;
  StaticRequestHandlerTest test_handler =
      StaticRequestHandlerTest(base_uri, root_path);
  Request req;
  Response res;
  NginxConfigParser parser = NginxConfigParser();
  NginxConfig config;
};

// TEST_F(StaticRequestHandlerTestFixture, CreateHandlerSingleSlash) {
//   std::unique_ptr<StaticRequestHandler> handler =
//       std::make_unique<StaticRequestHandler>("/static", "./");
//   EXPECT_NE(handler, nullptr);
// }

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test1/test.txt";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "line1\nline2\n\nline4");
}

TEST_F(StaticRequestHandlerTestFixture, ValidStaticRequestWithExtraSlashes) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test1/test.txt///";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 404);
  EXPECT_EQ(res.status_message, "Not Found");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "404 Not Found");
}

TEST_F(StaticRequestHandlerTestFixture, PdfFileRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test2/creeper.pdf";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "application/pdf");
}

TEST_F(StaticRequestHandlerTestFixture, ZipFileRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test1/creeper.zip";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "application/zip");
}

TEST_F(StaticRequestHandlerTestFixture, JPEGFileRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test1/creeper.jpg";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "image/jpeg");
}

TEST_F(StaticRequestHandlerTestFixture, HTMLFileRequest) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test2/test.html";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/html");
}

TEST_F(StaticRequestHandlerTestFixture, FileNotSupported) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test2/file_not_supported";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 200);
  EXPECT_EQ(res.status_message, "OK");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "application/octet-stream");
}

TEST_F(StaticRequestHandlerTestFixture, InvalidRequestOnFolder) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test2/empty";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 404);
  EXPECT_EQ(res.status_message, "Not Found");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "404 Not Found");
}

TEST_F(StaticRequestHandlerTestFixture, FileDoesNotExist) {
  req.valid = true;
  req.version = "HTTP/1.1";
  req.method = "GET";
  req.uri = "/static/test1/invalid_file.txt";

  res = test_handler.call_handle_request(req);

  EXPECT_EQ(res.status_code, 404);
  EXPECT_EQ(res.status_message, "Not Found");
  EXPECT_EQ(res.version, "HTTP/1.1");
  EXPECT_EQ(res.headers[0].name, "Content-Type");
  EXPECT_EQ(res.headers[0].value, "text/plain");
  EXPECT_EQ(res.body, "404 Not Found");
}

TEST_F(StaticRequestHandlerTestFixture, FilePathTextFile) {
  std::string file_path =
      test_handler.call_generate_file_path("/static/test1/test.txt");
  std::string extension = test_handler.call_get_file_content_type(file_path);
  EXPECT_EQ(file_path, "../data/test1/test.txt");
  EXPECT_EQ(extension, "text/plain");
}

TEST_F(StaticRequestHandlerTestFixture, FilePathTextFileWithTrailingSlash) {
  std::string file_path =
      test_handler.call_generate_file_path("/static/test1/test.txt//");
  std::string extension = test_handler.call_get_file_content_type(file_path);
  EXPECT_EQ(file_path, "../data/test1/test.txt//");
  EXPECT_EQ(extension, "application/octet-stream");
}

TEST_F(StaticRequestHandlerTestFixture, FilePathPdfFile) {
  std::string file_path =
      test_handler.call_generate_file_path("/static/test2/creeper.pdf");
  std::string extension = test_handler.call_get_file_content_type(file_path);
  EXPECT_EQ(file_path, "../data/test2/creeper.pdf");
  EXPECT_EQ(extension, "application/pdf");
}

TEST_F(StaticRequestHandlerTestFixture, GetType) {
  EXPECT_EQ(test_handler.get_type(),
            RequestHandler::HandlerType::STATIC_REQUEST_HANDLER);
}

TEST_F(StaticRequestHandlerTestFixture, ValidAbsolutePathArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_absolute_root", &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args->get_root_path(), "/usr/src/projects/creeper/data");
}

TEST_F(StaticRequestHandlerTestFixture, ValidRelativePathArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_relative_root", &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args->get_root_path(), "/usr/src/projects/creeper/data");
}

TEST_F(StaticRequestHandlerTestFixture, InvalidAbsolutePathArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_path_not_exist_absolute",
      &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(StaticRequestHandlerTestFixture, InvalidRelativePathArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_path_not_exist_relative",
      &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(StaticRequestHandlerTestFixture, InvalidRootArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_invalid_root", &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(StaticRequestHandlerTestFixture, MissingRootArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_missing_root", &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}

TEST_F(StaticRequestHandlerTestFixture, InvalidTrailingSlashArgs) {
  bool success = parser.parse(
      "request_handler_testcases/static_config_trailing_slash", &config);
  EXPECT_TRUE(success);
  args = StaticRequestHandlerArgs::create_from_config(config.statements_[0]);
  EXPECT_EQ(args, nullptr);
}
