#ifndef LOGGING_H
#define LOGGING_H

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <iostream>
#include <ostream>
#include <string>

namespace logging {

// Call this once at program startup.
// By default it installs a console sink to `std::clog`
// and a file sink to `logs/server_…`.
// In tests you can pass `nullptr` for console_stream to skip it,
// and empty `file_pattern` to skip file logging,
// or pass your own `std::ostream*` to capture logs.
void init_logging(
    std::ostream* console_stream = &std::clog,
    const std::string& file_pattern = "logs/server_%Y%m%d_%H%M%S_%N.log");

// Convenience macro to include file/line automatically
#define LOG(severity)         \
  BOOST_LOG_TRIVIAL(severity) \
      << "[" << __FILE__ << ":" << __LINE__ << "@" << __func__ << "] "

}  // namespace logging

#endif  // LOGGING_H
