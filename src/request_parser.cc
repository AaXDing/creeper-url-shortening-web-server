#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <unordered_set>
#include <iostream>
#include "request_parser.h"

std::string get_version(unsigned int version)
{
    return "HTTP/" + std::to_string(version / 10) + "." + std::to_string(version % 10);
}

const std::unordered_set<std::string> allowed_methods = {
    "GET" // only GET method is allowed for now
    /* future methods to be added:
    , "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH", "TRACE", "CONNECT"
    */
};

void request_parser::parse(request &req, const std::string &raw_request)
{
    // Store raw http request to parser
    boost::beast::flat_buffer buffer;
    buffer.commit(boost::asio::buffer_copy(buffer.prepare(raw_request.size()), boost::asio::buffer(raw_request)));

    boost::beast::http::request_parser<boost::beast::http::string_body> parser;
    boost::beast::error_code ec;

    parser.put(buffer.data(), ec);

    // If error in request, request not valid
    if (ec)
    {
        std::cout << ec.message() << std::endl;
        req.valid = false;
        return;
    }

    // Get parsed request
    auto res = parser.get();

    // Check request version HTTP/1.1 and request method allowed
    if (res.version() != 11 || allowed_methods.find(res.method_string()) == allowed_methods.end())
    {
        req.valid = false;
        return;
    }
    req.valid = true;
    req.version = get_version(res.version());
    req.uri = res.target();
    req.method = res.method_string();
    for (const auto &field : res)
    {
        req.headers.push_back(header{std::string(field.name_string()),
                                     std::string(field.value())});
    }
}
