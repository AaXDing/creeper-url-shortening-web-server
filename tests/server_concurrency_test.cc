#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <chrono>
#include <future>
#include <thread>
#include <vector>
#include <boost/thread.hpp>
#include "blocking_request_handler.h"
#include "config_parser.h"
#include "server.h"

class ServerConcurrencyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a config with a blocking handler
    NginxConfig config;
    NginxConfigParser parser;
    parser.parse("../my_config", &config);

    // Start the server with multiple worker threads
    io_service_ = std::make_shared<boost::asio::io_service>();
    server_ = std::make_unique<Server>(*io_service_, 8080, config);
    
    // Create a pool of worker threads
    unsigned int num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 4; // Default to 4 threads if hardware_concurrency() returns 0
    }
    
    std::vector<boost::thread> worker_threads;
    for (unsigned int i = 0; i < num_threads; ++i) {
        worker_threads.emplace_back([this]() {
            io_service_->run();
        });
    }
    
    // Store worker threads in class member for cleanup
    server_threads_ = std::move(worker_threads);
  }

  void TearDown() override {
    io_service_->stop();
    for (auto& thread : server_threads_) {
      thread.join();
    }
  }

  // Helper function to make an HTTP request
  std::string make_request(const std::string& path) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::socket socket(io_service);

    auto endpoints = resolver.resolve(
        "localhost", std::to_string(8080));  // port_ is set in SetUp()
    boost::asio::connect(socket, endpoints);

    std::string request = "GET " + path +
                          " HTTP/1.1\r\n"
                          "Host: localhost:" +
                          std::to_string(8080) +
                          "\r\n"
                          "Connection: close\r\n\r\n";

    boost::asio::write(socket, boost::asio::buffer(request));

    boost::asio::streambuf buf;
    boost::asio::read_until(socket, buf, "\r\n");  // read status‐line
    std::string head(boost::asio::buffers_begin(buf.data()),
                     boost::asio::buffers_end(buf.data()));
    return head;  // e.g. “HTTP/1.1 200 OK\r\n”
  }

  std::shared_ptr<boost::asio::io_service> io_service_;
  std::unique_ptr<Server> server_;
  std::vector<boost::thread> server_threads_;
};

TEST_F(ServerConcurrencyTest, ConcurrentRequests) {
  // Fire the slow one first (don’t wait on it yet)
  auto slow_future = std::async(std::launch::async, [this] {
    return make_request("/sleep");  // blocks ~5 s
  });

  // Give it 100 ms head start
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  // Time only the quick /health request
  auto t0 = std::chrono::steady_clock::now();
  auto fast_future = std::async(std::launch::async,
                                [this] { return make_request("/health"); });
  std::string fast_response = fast_future.get();
  auto t1 = std::chrono::steady_clock::now();

  EXPECT_LT(
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count(),
      1000); // 1000ms is the threshold for the health check
  EXPECT_NE(fast_response.find("200 OK"), std::string::npos);

  // Now wait for the sleeper so we don’t kill it early
  EXPECT_NE(slow_future.get().find("200 OK"), std::string::npos);
}

// TEST_F(ServerConcurrencyTest, MultipleConcurrentRequests) {
//     constexpr int k_slow = 2;
//     constexpr int k_fast = 3;

//     std::vector<std::future<std::string>> slow_futures;
//     std::vector<std::future<std::string>> fast_futures;

//     // fire-and-forget five blocking /sleep requests
//     for (int i = 0; i < k_slow; ++i)
//         slow_futures.emplace_back(std::async(std::launch::async,
//             [this]{ return make_request("/sleep"); }));

//     std::this_thread::sleep_for(std::chrono::milliseconds(100)); // give them time to enter the queue

//     // launch five /health requests and assert each finishes quickly
//     for (int i = 0; i < k_fast; ++i)
//         fast_futures.emplace_back(std::async(std::launch::async, [this]{
//             auto t0 = std::chrono::steady_clock::now();
//             auto resp = make_request("/health");
//             auto t1 = std::chrono::steady_clock::now();
//             EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count(),
//                       1000);
//             return resp;
//         }));

//     // verify fast responses first
//     for (auto& f : fast_futures)
//         EXPECT_NE(f.get().find("200 OK"), std::string::npos);

//     // then wait for the slow ones so the threads cleanly finish
//     for (auto& f : slow_futures)
//         EXPECT_NE(f.get().find("200 OK"), std::string::npos);
// }