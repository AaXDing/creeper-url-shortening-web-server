// Usage: ./server config_file
//
// Adopted from the Boost Asio example: async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2017 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <iostream>
#include <boost/thread.hpp>

#include "config_parser.h"
#include "logging.h"
#include "registry.h"
#include "server.h"

#define NUM_THREADS 4

int main(int argc, char* argv[]) {
  logging::init_logging();
  LOG(info) << "Logging initialized (CREEPER_LOG_DEBUG="
            << (std::getenv("CREEPER_LOG_DEBUG")
                    ? std::getenv("CREEPER_LOG_DEBUG")
                    : "unset")
            << ")";
  try {
    if (argc != 2) {
      LOG(error) << "Wrong invocation, need exactly one argument";
      throw std::runtime_error("Usage: server <config_file>");
    }

    boost::asio::io_service io_service;

    LOG(info) << "Starting server with config file: " << argv[1];

    NginxConfig config;
    NginxConfigParser parser;

    if (!parser.parse(argv[1], &config)) {
      LOG(error) << "Error parsing config file: " << argv[1];
      throw std::runtime_error("Error parsing config file");
    }

    LOG(info) << "Config parsed successfully";

    int port = config.get_port();
    if (port == -1) {
      LOG(error) << "No valid port found in config file";
      throw std::runtime_error("No valid port found in config file");
    }

    LOG(info) << "Creating server on port " << port;
    Server s(io_service, port, config);
    LOG(info) << "Server object constructed";

    // Create a pool of threads to run the io_service
    std::vector<boost::thread> threads;
    LOG(info) << "Starting " << NUM_THREADS << " worker threads";

    for (unsigned int i = 0; i < NUM_THREADS; ++i) {
      threads.emplace_back([&io_service]() {
        // io_service is captured by reference (&) because:
        // 1. All threads need to share the same io_service instance to coordinate work
        // 2. io_service is created in main() scope and outlives the threads
        // 3. We need to be able to stop the io_service from any thread
        try {
          io_service.run();
          boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
          signals.async_wait(
              [&](const boost::system::error_code& ec, int signal_number) {
                if (!ec) {
                  LOG(info) << "Signal " << signal_number
                            << " received, shutting down server";
                  io_service.stop();
                }
              });
        } catch (const std::exception& e) {
          LOG(error) << "Thread exception: " << e.what();
        }
      });
    }

    // Wait for all threads to complete
    for (auto& thread : threads) {
      thread.join();
    }

    LOG(info) << "Server terminated cleanly";
  } catch (std::exception& e) {
    LOG(fatal) << "Uncaught exception: " << e.what();
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  LOG(trace) << "Exiting application";
  return 0;
}
