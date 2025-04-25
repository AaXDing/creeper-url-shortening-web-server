#include "gtest/gtest.h"

#include "config_parser.h"
#include "http_header.h"
#include "request_handler_dispatcher.h"


class RequestHandlerDispatcherTestFixtrue : public ::testing::Test {
 protected:
  std::shared_ptr<RequestHandlerDispatcher> dispatcher;
  NginxConfig config;
  NginxConfigParser parser;
  Request req;
};

TEST_F(RequestHandlerDispatcherTestFixtrue, AddEchoHandler) {
  req.uri = "/echo";
  parser.parse("dispatcher_testcases/add_echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddEchoHandler2) {
  req.uri = "/echo/";
  parser.parse("dispatcher_testcases/add_echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddInvalidHandler) {
  req.uri = "/invalid";
  parser.parse("dispatcher_testcases/add_invalid_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 0);
  EXPECT_EQ(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, AddMultipleHandlers) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/add_multiple_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  EXPECT_EQ(dispatcher->get_num_handlers(), 2);
  EXPECT_NE(dispatcher->get_handler(req), nullptr);
}
