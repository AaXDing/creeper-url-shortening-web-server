// session.h
// A header file for the session class,
// which handles the communication with a single client.
#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include <memory>

#include "isession.h"
#include "request_handler_dispatcher.h"  // for dispatcher

class SessionTest;  // forward declaration for test fixture

class Session
    : public ISession,
      public std::enable_shared_from_this<Session> {  // Inherit from ISession
                                                      // Interface
 public:
  explicit Session(boost::asio::io_service &io_service,
                   std::shared_ptr<RequestHandlerDispatcher> dispatcher);

  // ISession interface -----------------------------------------------
  boost::asio::ip::tcp::socket &socket() override;
  void start() override;
  // -------------------------------------------------------------------
  friend class SessionTest;  // allow test fixture to access private members
 private:
  void handle_read(const boost::system::error_code &error,
                   size_t bytes_transferred);
  void handle_write(const boost::system::error_code &error);
  std::string handle_response(size_t bytes_transferred);

  boost::asio::ip::tcp::socket socket_;
  // max length of data buffer 1KB
  const static int MAX_LENGTH = 1024;
  char data_[MAX_LENGTH];

  std::shared_ptr<RequestHandlerDispatcher>
      dispatcher_;  // a constant reference to the dispatcher
};

#endif  // SESSION_H