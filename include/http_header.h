#ifndef HTTP_HEADER_H
#define HTTP_HEADER_H

#include <string>
#include <vector>

const std::string CRLF = "\r\n";
const std::string HTTP_VERSION = "HTTP/1.1";
const std::string METHOD_GET = "GET";

struct header
{
    std::string name;
    std::string value;
};

struct request
{
    // HTTP request line
    std::string method;
    std::string uri;
    std::string version;
    std::vector<header> headers;
    bool valid = false; // default valid to false
};

struct response
{
    // HTTP response line
    std::string version;
    int status_code;
    std::string status_message;
    std::string content_type;
    // std::string content_length;
    std::string body;
};

#endif