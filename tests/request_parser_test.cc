#include "request_parser.h"
#include "http_header.h"
#include "gtest/gtest.h"

class RequestParserTextFixture : public ::testing::Test
{
protected:
    request_parser parser;
    request req;
    std::string input;
};

TEST_F(RequestParserTextFixture, ParseSimpleValidRequest)
{
    input = "GET /index.html HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "User-Agent: curl/7.64.1\r\n"
            "Accept: */*\r\n"
            "\r\n";

    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::valid);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.uri, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.headers.size(), 3);
}

