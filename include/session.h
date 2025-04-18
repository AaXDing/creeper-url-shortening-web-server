// session.h
// A header file for the session class, 
// which handles the communication with a single client.
#ifndef SESSION_H
#define SESSION_H

#include <boost/asio.hpp>
#include "isession.h" 

class session : public ISession    // Inherit from ISession Interface
{
public:
    explicit session(boost::asio::io_service &io_service);

    // ISession interface -----------------------------------------------
    boost::asio::ip::tcp::socket &socket() override;
    void start() override;
    // -------------------------------------------------------------------

private:
    void handle_read(const boost::system::error_code &error,
                     size_t bytes_transferred);
    void handle_write(const boost::system::error_code &error);
    std::string handle_echo_response(const std::string &http_version,
                              const std::string &request_msg,
                              bool valid);

    boost::asio::ip::tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

#endif // SESSION_H