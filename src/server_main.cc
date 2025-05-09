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

#include "config_parser.h"
#include "logging.h"
#include "server.h"
#include "registry.h"

int main(int argc, char* argv[]) {
  logging::init_logging();
  LOG(info) << "Logging initialized (CREEPER_LOG_DEBUG="
            << (std::getenv("CREEPER_LOG_DEBUG")? std::getenv("CREEPER_LOG_DEBUG") : "unset")
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

    boost::asio::signal_set signals(io_service, SIGINT, SIGTERM);
    signals.async_wait(
      [&](const boost::system::error_code& ec, int signal_number) {
        if (!ec) {
          LOG(info) << "Signal " << signal_number
                    << " received, shutting down server";
          io_service.stop();
        }
      }
    );

    io_service.run();

    LOG(info) << "Server terminated cleanly";
  } catch (std::exception& e) {
    LOG(fatal) << "Uncaught exception: " << e.what();
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }
  
  LOG(trace) << "Exiting application";
  return 0;
}
