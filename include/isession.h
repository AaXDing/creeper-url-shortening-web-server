// isession.h
// Pure‑virtual interface so `server` can work with real, stub, or mock
// sessions.
#ifndef ISESSION_H
#define ISESSION_H

#include <boost/asio.hpp>
using boost::asio::ip::tcp;

class ISession {
 public:
  virtual tcp::socket& socket() = 0;  // expose the underlying socket
  virtual void start() = 0;           // kick off the read / write loop
  virtual ~ISession() = default;      // always give polymorphic base a v‑dtor
  virtual tcp::endpoint remote_endpoint() {
    // by default just forward to the real socket
    return socket().remote_endpoint();
  }
};

#endif  // ISESSION_H
