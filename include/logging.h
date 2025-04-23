// logging.h
#ifndef LOGGING_H
#define LOGGING_H

#include <boost/log/trivial.hpp>

// Call this once at program startup (before spawning threads or accepting connections)
void init_logging();

// Convenience macro to include file/line automatically
#define LOG(severity) BOOST_LOG_TRIVIAL(severity) \
    << "[" << __FILE__ << ":" << __LINE__ << "@" << __func__ << "] "


#endif  // LOGGING_H