#include "config_parser.h"

#include "boost/filesystem.hpp"
#include "echo_request_handler.h"
#include "gtest/gtest.h"
#include "not_found_request_handler.h"
#include "registry.h"
#include "static_request_handler.h"

// below are tests added using test fixture
class NginxConfigParserTestFixture : public testing::Test {
 protected:
  NginxConfigParser parser;
  NginxConfig config;
  NginxLocationResult result;
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

TEST_F(NginxConfigParserTestFixture, GetValidEchoLocations) {
  bool success = parser.parse("config_testcases/echo_handler_on_root", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_TRUE(result.valid);
  EXPECT_EQ(result.locations.size(), 1);
  EXPECT_EQ(result.locations[0].path, "/");
  EXPECT_EQ(result.locations[0].handler, "EchoHandler");
}

TEST_F(NginxConfigParserTestFixture, GetValidStaticLocations) {
  bool success = parser.parse("config_testcases/static_handler", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_TRUE(result.valid);
  EXPECT_EQ(result.locations.size(), 1);
  EXPECT_EQ(result.locations[0].path, "/static");
  EXPECT_EQ(result.locations[0].handler, "StaticHandler");
  EXPECT_TRUE(result.locations[0].root.has_value());
  EXPECT_EQ(result.locations[0].root.value(),
            boost::filesystem::canonical("../data").string());
}

TEST_F(NginxConfigParserTestFixture, GetMultipleValidLocations) {
  bool success = parser.parse("config_testcases/multiple_handlers", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_TRUE(result.valid);
  EXPECT_EQ(result.locations.size(), 5);

  // Verify each location
  bool found_echo = false;
  bool found_static = false;
  bool found_static1 = false;
  for (const auto& loc : result.locations) {
    if (loc.path == "/echo" && loc.handler == "EchoHandler") {
      found_echo = true;
    }
    if (loc.path == "/static" && loc.handler == "StaticHandler") {
      found_static = true;
      EXPECT_TRUE(loc.root.has_value());
      EXPECT_EQ(loc.root.value(),
                boost::filesystem::canonical("../data/test1").string());
    }
    if (loc.path == "/static1" && loc.handler == "StaticHandler") {
      found_static1 = true;
      EXPECT_TRUE(loc.root.has_value());
      EXPECT_EQ(loc.root.value(),
                boost::filesystem::canonical("../data/test2").string());
    }
  }
  EXPECT_TRUE(found_echo);
  EXPECT_TRUE(found_static);
  EXPECT_TRUE(found_static1);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithInvalidPath) {
  bool success = parser.parse("config_testcases/invalid_path", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithQuotedPath) {
  bool success = parser.parse("config_testcases/quoted_path", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithQuotedRootPath) {
  bool success = parser.parse("config_testcases/quoted_root_path", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithDuplicatePath) {
  bool success = parser.parse("config_testcases/duplicate_handlers", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithInvalidHandler) {
  bool success = parser.parse("config_testcases/invalid_handler_type", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithInvalidStaticConfig) {
  bool success =
      parser.parse("config_testcases/invalid_static_config", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithInvalidEchoConfig) {
  bool success = parser.parse("config_testcases/invalid_echo_config", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, GetLocationsWithTrailingSlash) {
  bool success = parser.parse("config_testcases/trailing_slash", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, ValidGetLocationsWithAbsoluteRootPath) {
  bool success = parser.parse("config_testcases/absolute_root_path", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_TRUE(result.valid);
  EXPECT_EQ(result.locations.size(), 1);
  EXPECT_EQ(result.locations[0].root.value(), "/usr/src/projects/creeper/data");
}

TEST_F(NginxConfigParserTestFixture, InvalidGetLocationsWithAbsoluteRootPath) {
  bool success =
      parser.parse("config_testcases/invalid_absolute_root_path", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}

TEST_F(NginxConfigParserTestFixture, ValidGetLocationsWithNotFoundHandler) {
  bool success = parser.parse("config_testcases/not_found_handler", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_TRUE(result.valid);
  EXPECT_EQ(result.locations.size(), 1);
  EXPECT_EQ(result.locations[0].path, "/");
  EXPECT_EQ(result.locations[0].handler, "NotFoundHandler");
}

TEST_F(NginxConfigParserTestFixture, InvalidGetLocationsWithNotFoundHandler) {
  bool success =
      parser.parse("config_testcases/invalid_not_found_handler", &config);
  EXPECT_TRUE(success);
  result = config.get_locations();
  EXPECT_FALSE(result.valid);
  EXPECT_EQ(result.locations.size(), 0);
}