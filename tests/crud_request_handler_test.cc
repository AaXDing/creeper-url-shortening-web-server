#include "crud_request_handler.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>

// Utility to create a fake Request
Request make_request(const std::string &method, const std::string &uri,
                     const std::string &body = "") {
  Request req;
  req.method = method;
  req.uri = uri;
  req.version = "HTTP/1.1";
  req.valid = true;
  req.body = body;
  return req;
}

class CrudRequestHandlerTestFixture : public ::testing::Test {
protected:
  std::string test_dir = "./test_crud_data";
  std::string base_uri = "/api";
  CrudRequestHandler handler{base_uri, test_dir};

  void SetUp() override {
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir); }
};

TEST_F(CrudRequestHandlerTestFixture, ExtractEntityParsesCorrectly) {
  EXPECT_EQ(handler.extract_entity(base_uri + "/Books/3"), "Books");
  EXPECT_EQ(handler.extract_entity(base_uri + "/Shoes"), "Shoes");
  EXPECT_EQ(handler.extract_entity("/notapi/Shoes/1"), "");
  EXPECT_EQ(handler.extract_entity(base_uri), "");
  EXPECT_EQ(handler.extract_entity(base_uri), "");
}

TEST_F(CrudRequestHandlerTestFixture, ExtractIdParsesCorrectly) {
  EXPECT_EQ(handler.extract_id(base_uri + "/Books/3"), "3");
  EXPECT_EQ(handler.extract_id(base_uri + "/Shoes/77"), "77");
  EXPECT_EQ(handler.extract_id(base_uri + "/Shoes/"), "");
  EXPECT_EQ(handler.extract_id(base_uri + "/Shoes"), "");
  EXPECT_EQ(handler.extract_id("/notapi/Shoes/1"), "");
}

TEST_F(CrudRequestHandlerTestFixture, GetNextAvailableIdSkipsNonNumeric) {
  std::string entity_dir = test_dir + "/Hats";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/1") << "{}";
  std::ofstream(entity_dir + "/7") << "{}";
  std::ofstream(entity_dir + "/abc") << "{}"; // should be skipped
  std::ofstream(entity_dir + "/999") << "{}";

  int next_id = handler.get_next_available_id(entity_dir);
  EXPECT_EQ(next_id, 1000);
}

TEST_F(CrudRequestHandlerTestFixture, GetNextAvailableIdHandlesEmptyDir) {
  std::string entity_dir = test_dir + "/Empty";
  std::filesystem::create_directories(entity_dir);

  int next_id = handler.get_next_available_id(entity_dir);
  EXPECT_EQ(next_id, 1);
}

TEST_F(CrudRequestHandlerTestFixture, ListIdsReturnsAllFilenamesAsJsonArray) {
  std::string entity_dir = test_dir + "/Games";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/5") << "{}";
  std::ofstream(entity_dir + "/42") << "{}";
  std::ofstream(entity_dir + "/hello.txt") << "{}"; // should still appear

  std::string result = handler.list_ids(entity_dir);
  EXPECT_TRUE(result.find("\"5\"") != std::string::npos);
  EXPECT_TRUE(result.find("\"42\"") != std::string::npos);
  EXPECT_TRUE(result.find("\"hello.txt\"") != std::string::npos);
}

TEST_F(CrudRequestHandlerTestFixture, PostCreatesNewEntityFile) {
  auto req = make_request("POST", base_uri + "/Books", R"({"title":"1984"})");

  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");
  EXPECT_TRUE(res->body.find("\"id\":") != std::string::npos);

  // Check file exists
  std::string id = res->body.substr(res->body.find(":") + 1);
  id.erase(remove(id.begin(), id.end(), '}'), id.end());
  std::string file_path = test_dir + "/Books/" + std::to_string(std::stoi(id));
  EXPECT_TRUE(std::filesystem::exists(file_path));
}

TEST_F(CrudRequestHandlerTestFixture, GetReturnsEntityContents) {
  std::string entity_dir = test_dir + "/Shoes";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/1") << R"({"size":42})";

  auto req = make_request("GET", base_uri + "/Shoes/1");
  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");
  EXPECT_EQ(res->body, R"({"size":42})");
}

TEST_F(CrudRequestHandlerTestFixture, GetReturnsListOfEntityIds) {
  std::string entity_dir = test_dir + "/Books";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/1") << R"({"title":"A"})";
  std::ofstream(entity_dir + "/2") << R"({"title":"B"})";
  std::ofstream(entity_dir + "/99") << R"({"title":"Z"})";

  auto req = make_request("GET", base_uri + "/Books");
  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");

  // Check that expected IDs are in the returned list
  std::string body = res->body;
  EXPECT_TRUE(body.find("\"1\"") != std::string::npos);
  EXPECT_TRUE(body.find("\"2\"") != std::string::npos);
  EXPECT_TRUE(body.find("\"99\"") != std::string::npos);
}

TEST_F(CrudRequestHandlerTestFixture, GetNonexistentEntityIdReturns404) {
  std::string entity_dir = test_dir + "/Books";
  std::filesystem::create_directories(entity_dir);
  // No file for ID 999

  auto req = make_request("GET", base_uri + "/Books/999");
  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}

TEST_F(CrudRequestHandlerTestFixture, GetFromNonexistentEntityTypeReturns404) {
  // No /Music directory created

  auto req = make_request("GET", base_uri + "/Music/1");
  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}

TEST_F(CrudRequestHandlerTestFixture, GetInvalidIdReturns404) {
  std::string entity_dir = test_dir + "/Games";
  std::filesystem::create_directories(entity_dir);

  // No such ID file
  auto req = make_request("GET", base_uri + "/Games/thisisnotanumber");
  auto res = handler.handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}