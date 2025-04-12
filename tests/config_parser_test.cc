#include "gtest/gtest.h"
#include "config_parser.h"

TEST(NginxConfigParserTest, SimpleConfig) {
  NginxConfigParser parser;
  NginxConfig out_config;

  bool success = parser.Parse("example_config", &out_config);

  EXPECT_TRUE(success);
}

// below are tests added using test fixture
class NginxConfigParserTestFixture : public testing::Test {
  protected:
    NginxConfigParser parser;
    NginxConfig config; 
};

// provided test using fixture syntax
TEST_F(NginxConfigParserTestFixture, SimpleConfig) {
  bool success = parser.Parse("example_config", &config);
  EXPECT_TRUE(success);
}

TEST_F(NginxConfigParserTestFixture, CommentConfig) {
  bool success = parser.Parse("comment_config", &config);
  EXPECT_TRUE(success);
}

// test with nested blocks
TEST_F(NginxConfigParserTestFixture, NestedBlockConfig) {
  bool success = parser.Parse("nested_block_config", &config);
  EXPECT_TRUE(success);
}

// test unclosed start bracket
TEST_F(NginxConfigParserTestFixture, UnclosedBlockConfig) {
  bool success = parser.Parse("unclosed_block_config", &config);
  EXPECT_FALSE(success);
}

// test unclosed end bracket
TEST_F(NginxConfigParserTestFixture, UnclosedBlockConfig2) {
  bool success = parser.Parse("unclosed_block_config2", &config);
  EXPECT_FALSE(success);
}

// test block with no statements
TEST_F(NginxConfigParserTestFixture, EmptyBlockConfig) {
  bool success = parser.Parse("empty_block_config", &config);
  EXPECT_TRUE(success);
}