#include "config_parser.h"

#include "gtest/gtest.h"

// below are tests added using test fixture
class NginxConfigParserTestFixture : public testing::Test {
 protected:
  NginxConfigParser parser;
  NginxConfig config;
};

// provided test using fixture syntax
TEST_F(NginxConfigParserTestFixture, SimpleConfig) {
  bool success = parser.parse("config_testcases/simple_config", &config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, CommentConfig) {
  bool success = parser.parse("config_testcases/comment_config", &config);
  EXPECT_TRUE(success);
}

// test with nested blocks
TEST_F(NginxConfigParserTestFixture, NestedBlockConfig) {
  bool success = parser.parse("config_testcases/nested_block_config", &config);
  EXPECT_TRUE(success);
}

// test block with no statements
TEST_F(NginxConfigParserTestFixture, EmptyBlockConfig) {
  bool success = parser.parse("config_testcases/empty_block_config", &config);
  EXPECT_TRUE(success);
}

// test block with single quote
TEST_F(NginxConfigParserTestFixture, SingleQuoteConfig) {
  bool success = parser.parse("config_testcases/single_quote_config", &config);
  EXPECT_TRUE(success);
}

// test unclosed start bracket
TEST_F(NginxConfigParserTestFixture, UnclosedBlockConfig) {
  bool success =
      parser.parse("config_testcases/unclosed_block_config", &config);
  EXPECT_FALSE(success);
}

// test unclosed end bracket
TEST_F(NginxConfigParserTestFixture, UnclosedBlockConfig2) {
  bool success =
      parser.parse("config_testcases/unclosed_block_config2", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ConfigNotFound) {
  bool success = parser.parse("config_testcases/no_such_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, UnclosedSingleQuote) {
  bool success =
      parser.parse("config_testcases/unclosed_single_quote_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, EmptyConfig) {
  bool success = parser.parse("config_testcases/empty_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, MissingSemicolonConfig) {
  bool success =
      parser.parse("config_testcases/missing_semicolon_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ExtraSemicolonConfig) {
  bool success =
      parser.parse("config_testcases/extra_semicolon_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, BlockAfterSemicolonConfig) {
  bool success =
      parser.parse("config_testcases/block_after_semicolon_config", &config);
  EXPECT_FALSE(success);
}

TEST_F(NginxConfigParserTestFixture, ToStringMethod) {
  bool success = parser.parse("config_testcases/nested_block_config", &config);
  EXPECT_TRUE(success);
  std::string expected_output =
      "server {\n"
      "  port 80;\n"
      "  location /static StaticHandler {\n"
      "    root ./static;\n"
      "  }\n"
      "}\n";
  std::string actual_output = config.to_string();
  EXPECT_EQ(expected_output, actual_output);
}

TEST_F(NginxConfigParserTestFixture, GetValidPort) {
  bool success = parser.parse("config_testcases/nested_block_config", &config);
  EXPECT_TRUE(success);
  int port = config.get_port();
  EXPECT_EQ(port, 80);
}

TEST_F(NginxConfigParserTestFixture, GetInvalidPort) {
  bool success = parser.parse("config_testcases/invalid_port_config", &config);
  EXPECT_TRUE(success);
  int port = config.get_port();
  EXPECT_EQ(port, -1);
}

TEST_F(NginxConfigParserTestFixture, GetPortWithNoPort) {
  bool success = parser.parse("config_testcases/empty_block_config", &config);
  EXPECT_TRUE(success);
  int port = config.get_port();
  EXPECT_EQ(port, -1);
}