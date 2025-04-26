#include "request_handler_dispatcher.h"

#include "config_parser.h"
#include "gtest/gtest.h"
#include "http_header.h"

class RequestHandlerDispatcherTestFixtrue : public ::testing::Test {
 protected:
  std::shared_ptr<RequestHandlerDispatcher> dispatcher;
  NginxConfig config;
  NginxConfigParser parser;
  Request req;
};

TEST_F(RequestHandlerDispatcherTestFixtrue, AddEchoHandler) {
  req.uri = "/echo";
  parser.parse("dispatcher_testcases/echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddInvalidHandler) {
  req.uri = "/invalid";
  parser.parse("dispatcher_testcases/invalid_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 0);
  EXPECT_EQ(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddStaticHandler) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/static_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddEchoHandlerWithTrailingSlash) {
  req.uri = "/echo";
  parser.parse("dispatcher_testcases/echo_with_trailing_slashes_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddDuplicateHandlers) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/duplicate_handlers", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddMultipleHandlers) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/multiple_handlers", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 2);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, GetEchoHandlerWithTrailingSlash) {
  req.uri = "/echo///";
  parser.parse("dispatcher_testcases/echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}