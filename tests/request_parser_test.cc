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

TEST_F(RequestParserTextFixture, SimpleRequest)
{
    input = "GET /index.html HTTP/1.1\r\n"
            "Host: www.example.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::valid);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.uri, "/index.html");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.headers.size(), 1);
}

TEST_F(RequestParserTextFixture, RequestWithNoHeader)
{
    input = "GET /no-header HTTP/1.1\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::valid);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.uri, "/no-header");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.headers.size(), 0);
}

TEST_F(RequestParserTextFixture, RequestWithExtraHeaders)
{
    input = "GET /home HTTP/1.1\r\n"
            "Host: test.com\r\n"
            "Connection: keep-alive\r\n"
            "Accept-Encoding: gzip, deflate\r\n"
            "Accept-Language: en-US\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::valid);
    EXPECT_EQ(req.method, "GET");
    EXPECT_EQ(req.uri, "/home");
    EXPECT_EQ(req.version, "HTTP/1.1");
    EXPECT_EQ(req.headers.size(), 4);
}

TEST_F(RequestParserTextFixture, InvalidMethodRequest)
{
    input = "FETCH /weird HTTP/1.1\r\n"
            "Host: weird.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MissingMethodRequest)
{
    input = " /weird HTTP/1.1\r\n"
            "Host: weird.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MissingSpaceRequest)
{
    input = "GET/nospace HTTP/1.1\r\n"
            "Host: www.nospace.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MissingHttpVersion)
{
    input = "GET /no-version\r\n"
            "Host: noversion.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, WrongHttpVersion)
{
    input = "GET /wrong-version HTTP/2.0\r\n"
            "Host: wrongversion.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MalformedRequest)
{
    input = "GET: /malformed HTTP/1.1\r\n"
            "Host: malformed.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MalformedHeader)
{
    input = "GET /malformed HTTP/1.1\r\n"
            "Host malformed.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, EmptyRequest)
{
    input = "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, IncompleteRequest)
{
    input = "GET /incomplete HTTP/1.1\r\n"
            "Host: incomplete.com\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::indeterminate);
}

TEST_F(RequestParserTextFixture, MissingNewlineRequest1)
{
    input = "GET /incomplete HTTP/1.1"
            "Host: incomplete.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MissingNewlineRequest2)
{
    input = "GET /incomplete HTTP/1.1\r"
            "Host: incomplete.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}

TEST_F(RequestParserTextFixture, MissingNewlineRequest3)
{
    input = "GET /incomplete HTTP/1.1\n"
            "Host: incomplete.com\r\n"
            "\r\n";
    auto result = parser.parse(req, input.begin(), input.end());
    EXPECT_EQ(std::get<0>(result), request_parser::invalid);
}