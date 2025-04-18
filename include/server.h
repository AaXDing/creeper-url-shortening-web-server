// server.h
// A header file for the server class,
// which accepts incoming connections and starts a session for each client.
#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include "isession.h"   // Include the interface for session

using boost::asio::ip::tcp;

class server
{
public:
    /// Factory: returns a heap‑allocated ISession ready to accept().
    using SessionFactory = std::function<std::unique_ptr<ISession>()>;

    server(boost::asio::io_service& io,
        short port,
        SessionFactory fac = nullptr);

private:
    void start_accept();
    void handle_accept(ISession* sess, const boost::system::error_code& ec);

    boost::asio::io_service& io_;
    tcp::acceptor            acceptor_;
    SessionFactory           make_session_;
};

#endif // SERVER_H