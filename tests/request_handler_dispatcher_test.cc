#include "request_handler_dispatcher.h"

#include "config_parser.h"
#include "gtest/gtest.h"
#include "http_header.h"
#include "registry.h"
#include <iostream>
#include "echo_request_handler.h"
#include "static_request_handler.h"

class RequestHandlerDispatcherTestFixtrue : public ::testing::Test {
 protected:
  std::shared_ptr<RequestHandlerDispatcher> dispatcher;
  NginxConfig config;
  NginxConfigParser parser;
  Request req;
  
};

TEST_F(RequestHandlerDispatcherTestFixtrue, EchoHandler) {
  std::cerr << Registry::get_factory_map().size() << std::endl;

  req.uri = "/echo";
  parser.parse("dispatcher_testcases/echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  //EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_EQ(dispatcher->get_handler(req)->get_type(),
            RequestHandler::HandlerType::ECHO_REQUEST_HANDLER);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, InvalidHandler) {
  req.uri = "/invalid";
  parser.parse("dispatcher_testcases/invalid_handler", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
  // EXPECT_EQ(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, StaticHandler) {
  req.uri = "/static1";
  parser.parse("dispatcher_testcases/static_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  //EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_EQ(dispatcher->get_handler(req)->get_type(),
            RequestHandler::HandlerType::STATIC_REQUEST_HANDLER);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, EchoHandlerWithTrailingSlash) {
  req.uri = "/echo";
  parser.parse("dispatcher_testcases/echo_with_trailing_slashes_handler",
               &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, DuplicateHandlers) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/duplicate_handlers", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
  // EXPECT_EQ(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, MultipleHandlers) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/multiple_handlers", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  //EXPECT_EQ(dispatcher->get_num_handlers(), 5);
  EXPECT_EQ(dispatcher->get_handler(req)->get_type(),
            RequestHandler::HandlerType::STATIC_REQUEST_HANDLER);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, GetEchoHandlerWithTrailingSlash) {
  req.uri = "/echo///";
  parser.parse("dispatcher_testcases/echo_handler", &config);
  dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  //EXPECT_EQ(dispatcher->get_num_handlers(), 1);
  EXPECT_EQ(dispatcher->get_handler(req)->get_type(),
            RequestHandler::HandlerType::ECHO_REQUEST_HANDLER);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, InvalidStaticHandlerFilePath) {
  req.uri = "/static";
  parser.parse("dispatcher_testcases/invalid_static_file_path", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
  // EXPECT_EQ(dispatcher->get_handler(req), nullptr);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, MissingRootPath) {
  parser.parse("dispatcher_testcases/missing_root_path", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, MissingRootKeyword) {
  parser.parse("dispatcher_testcases/missing_root_keyword", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, NoHandlerType) {
  parser.parse("dispatcher_testcases/no_handler_type", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, InvalidHandlerFormat) {
  parser.parse("dispatcher_testcases/invalid_handler_format", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}

TEST_F(RequestHandlerDispatcherTestFixtrue, QuotedRootPath) {
  parser.parse("dispatcher_testcases/quoted_root_path", &config);
  EXPECT_THROW(dispatcher = std::make_shared<RequestHandlerDispatcher>(config),
               std::runtime_error);
  // dispatcher = std::make_shared<RequestHandlerDispatcher>(config);
  // EXPECT_EQ(dispatcher->get_num_handlers(), 0);
}