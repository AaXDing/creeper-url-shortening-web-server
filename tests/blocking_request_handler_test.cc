#include "blocking_request_handler.h"

#include <gtest/gtest.h>

#include <chrono>

#include "http_header.h"

class BlockingRequestHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create handler args
    std::shared_ptr<NginxConfigStatement> stmt =
        std::make_shared<NginxConfigStatement>();
    auto args_ = BlockingRequestHandlerArgs::create_from_config(stmt);
    // Create the blocking handler
    handler_ = std::make_unique<BlockingRequestHandler>("/sleep", args_);
  }

  std::shared_ptr<BlockingRequestHandlerArgs> args_;
  std::unique_ptr<BlockingRequestHandler> handler_;
};

TEST_F(BlockingRequestHandlerTest, BlockingRequestHandlerTest) {
  Request request;
  request.method = "GET";
  request.uri = "/sleep";
  request.version = "HTTP/1.1";
  request.headers = {};
  request.body = "";

  auto start = std::chrono::high_resolution_clock::now();
  auto response = handler_->handle_request(request);
  auto end = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
  EXPECT_EQ(duration.count(), DEFAULT_SLEEP_DURATION_SECONDS);
}

TEST_F(BlockingRequestHandlerTest, BlockingHandlerType) {
  EXPECT_EQ(handler_->get_type(),
            RequestHandler::HandlerType::BLOCKING_REQUEST_HANDLER);
}