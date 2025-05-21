#include "crud_request_handler.h"
#include "gtest/gtest.h"
#include "sim_entity_storage.h"

#include <filesystem>
#include <fstream>

// Utility to create a fake Request
Request make_request(const std::string &method, const std::string &uri,
                     const std::string &body = "",
                     const std::vector<Header> &headers = {})
{
  Request req;
  req.method = method;
  req.uri = uri;
  req.version = "HTTP/1.1";
  req.valid = true;
  req.body = body;
  req.headers = headers;
  return req;
}

class CrudRequestHandlerTestFixture : public ::testing::Test
{
protected:
  std::shared_ptr<SimEntityStorage> sim_entity_storage;
  std::string test_dir = "./test_crud_data";
  std::string base_uri = "/api";
  std::unique_ptr<CrudRequestHandler> handler;

  void SetUp() override
  {
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    sim_entity_storage = std::make_shared<SimEntityStorage>();
    handler = std::make_unique<CrudRequestHandler>(base_uri, test_dir,
                                                   sim_entity_storage);
  }

  void TearDown() override { std::filesystem::remove_all(test_dir); }
};

TEST_F(CrudRequestHandlerTestFixture, ExtractEntityParsesCorrectly)
{
  EXPECT_EQ(handler->extract_entity(base_uri + "/Books/3"), "Books");
  EXPECT_EQ(handler->extract_entity(base_uri + "/Shoes"), "Shoes");
  EXPECT_EQ(handler->extract_entity("/notapi/Shoes/1"), "");
  EXPECT_EQ(handler->extract_entity(base_uri), "");
  EXPECT_EQ(handler->extract_entity(base_uri), "");
}

TEST_F(CrudRequestHandlerTestFixture, ExtractIdParsesCorrectly)
{
  EXPECT_EQ(handler->extract_id(base_uri + "/Books/3"), "3");
  EXPECT_EQ(handler->extract_id(base_uri + "/Shoes/77"), "77");
  EXPECT_EQ(handler->extract_id(base_uri + "/Shoes/"), "");
  EXPECT_EQ(handler->extract_id(base_uri + "/Shoes"), "");
  EXPECT_EQ(handler->extract_id("/notapi/Shoes/1"), "");
}

TEST_F(CrudRequestHandlerTestFixture, GetNextAvailableIdSkipsNonNumeric)
{
  std::string entity_dir = test_dir + "/Hats";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/1") << "{}";
  std::ofstream(entity_dir + "/7") << "{}";
  std::ofstream(entity_dir + "/abc") << "{}"; // should be skipped
  std::ofstream(entity_dir + "/999") << "{}";

  int next_id = handler->get_next_available_id(entity_dir);
  EXPECT_EQ(next_id, 1000);
}

TEST_F(CrudRequestHandlerTestFixture, GetNextAvailableIdHandlesEmptyDir)
{
  std::string entity_dir = test_dir + "/Empty";
  std::filesystem::create_directories(entity_dir);

  int next_id = handler->get_next_available_id(entity_dir);
  EXPECT_EQ(next_id, 1);
}

TEST_F(CrudRequestHandlerTestFixture, ListIdsReturnsAllFilenamesAsJsonArray)
{
  std::string entity_dir = test_dir + "/Games";
  std::filesystem::create_directories(entity_dir);
  std::ofstream(entity_dir + "/5") << "{}";
  std::ofstream(entity_dir + "/42") << "{}";
  std::ofstream(entity_dir + "/hello.txt") << "{}"; // should still appear

  std::string result = handler->list_ids(entity_dir);
  EXPECT_TRUE(result.find("\"5\"") != std::string::npos);
  EXPECT_TRUE(result.find("\"42\"") != std::string::npos);
  EXPECT_TRUE(result.find("\"hello.txt\"") != std::string::npos);
}

TEST_F(CrudRequestHandlerTestFixture, PostValidJsonReturns201)
{
  auto req = make_request("POST", base_uri + "/Books", R"({"title":"Valid"})",
                          {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 201);
  EXPECT_EQ(res->status_message, "Created");
}

TEST_F(CrudRequestHandlerTestFixture, PostWorksWithLowercaseHeader)
{
  auto req = make_request("POST", base_uri + "/Books", R"({"title":"Valid"})",
                          {{"content-type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 201);
  EXPECT_EQ(res->status_message, "Created");
}

TEST_F(CrudRequestHandlerTestFixture, PostMissingContentTypeReturns415)
{
  auto req =
      make_request("POST", base_uri + "/Books", R"({"title":"Missing"})");

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 415);
  EXPECT_EQ(res->status_message, "Unsupported Media Type");
}

TEST_F(CrudRequestHandlerTestFixture, PostWrongContentTypeReturns415)
{
  auto req = make_request("POST", base_uri + "/Books", R"({"title":"Wrong"})",
                          {{"Content-Type", "text/plain"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 415);
}

TEST_F(CrudRequestHandlerTestFixture, PostEmptyBodyReturns400)
{
  auto req = make_request("POST", base_uri + "/Books", "",
                          {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 400);
  EXPECT_EQ(res->status_message, "Bad Request");
}

TEST_F(CrudRequestHandlerTestFixture, PostMalformedJsonReturns400)
{
  auto req = make_request("POST", base_uri + "/Books", "{ bad json",
                          {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 400);
  EXPECT_EQ(res->status_message, "Bad Request");
}

TEST_F(CrudRequestHandlerTestFixture, GetReturnsEntityContents)
{
  auto req = make_request("POST", base_uri + "/Shoes",
                          R"({"size":42})",
                          {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);

  req = make_request("GET", base_uri + "/Shoes/1");
  res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");
  EXPECT_EQ(res->body, R"({"size":42})");
}

TEST_F(CrudRequestHandlerTestFixture, GetReturnsListOfEntityIds)
{
  auto req = make_request("PUT", base_uri + "/Books/1",
                          R"({"title":"A"})",
                          {{"Content-Type", "application/json"}});
  auto res = handler->handle_request(req);
  req = make_request("PUT", base_uri + "/Books/2",
                          R"({"title":"B"})",
                          {{"Content-Type", "application/json"}});
  res = handler->handle_request(req);
  req = make_request("PUT", base_uri + "/Books/99",
                          R"({"title":"Z"})",
                          {{"Content-Type", "application/json"}});
  res = handler->handle_request(req);

  req = make_request("GET", base_uri + "/Books");
  res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");

  // Check that expected IDs are in the returned list
  std::string body = res->body;
  EXPECT_TRUE(body.find("\"1\"") != std::string::npos);
  EXPECT_TRUE(body.find("\"2\"") != std::string::npos);
  EXPECT_TRUE(body.find("\"99\"") != std::string::npos);
}

TEST_F(CrudRequestHandlerTestFixture, GetNonexistentEntityIdReturns404)
{
  std::string entity_dir = test_dir + "/Books";
  // No file for ID 999
  auto req = make_request("GET", base_uri + "/Books/999");
  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}

TEST_F(CrudRequestHandlerTestFixture, GetFromNonexistentEntityTypeReturns404)
{
  // No /Music directory created

  auto req = make_request("GET", base_uri + "/Music/1");
  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}

TEST_F(CrudRequestHandlerTestFixture, GetInvalidIdReturns404)
{
  std::string entity_dir = test_dir + "/Games";
  std::filesystem::create_directories(entity_dir);

  // No such ID file
  auto req = make_request("GET", base_uri + "/Games/thisisnotanumber");
  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
}

TEST_F(CrudRequestHandlerTestFixture, GetNonexistentEntityListReturns404)
{
  auto req = make_request(
      "GET", base_uri + "/Ghosts"); // /api/Ghosts (directory doesn't exist)

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 404);
  EXPECT_EQ(res->status_message, "Not Found");
  EXPECT_EQ(res->body, "Entity type not found");
}

TEST_F(CrudRequestHandlerTestFixture, CreateNonExistantEntityWithPUT)
{
  std::string id = "420";
  auto req = make_request(
      "PUT", base_uri + "/Movies/" + id, R"({"title":"Wall-E", "rating": 10})",
      {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 201);

  // Check file exists
  req = make_request("GET", base_uri + "/Movies/" + id);
  res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 200);
}

TEST_F(CrudRequestHandlerTestFixture, UpdateExistingEntityWithPUT)
{
  std::string id = "69";
  std::string entity_dir = test_dir + "/Movies";
  auto req = make_request(
      "PUT", base_uri + "/Movies/" + id, R"({"title":"Up", "rating": 9.5})",
      {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 201);

  req = make_request(
      "PUT", base_uri + "/Movies/" + id, R"({"title":"Cars", "rating": 9.2})",
      {{"Content-Type", "application/json"}});

  res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->body, "");

  req = make_request("GET", base_uri + "/Movies/" + id);
  res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 200);
  EXPECT_EQ(res->content_type, "application/json");
  EXPECT_EQ(res->body, R"({"title":"Cars","rating":9.2E0})");
}

TEST_F(CrudRequestHandlerTestFixture, PutMalformedJsonReturns400)
{
  auto req = make_request("PUT", base_uri + "/Movies/69", "{ bad json",
                          {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 400);
  EXPECT_EQ(res->status_message, "Bad Request");
}

TEST_F(CrudRequestHandlerTestFixture, PutNoIDReturns405)
{
  auto req = make_request(
      "PUT", base_uri + "/Movies", R"({"title":"Up", "rating": 9.5})",
      {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 405);
}

TEST_F(CrudRequestHandlerTestFixture, DeleteExistingEntity)
{
  std::string id = "420";
  std::string entity_dir = test_dir + "/Movies";
  auto req = make_request(
      "PUT", base_uri + "/Movies/" + id, R"({"title":"Wall-E", "rating": 10})",
      {{"Content-Type", "application/json"}});

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 201);

  // Check file exists
  req = make_request("GET", base_uri + "/Movies/" + id);
  res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 200);

  req = make_request("DELETE", base_uri + "/Movies/" + id);

  res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 204);

  // Check file exists
  req = make_request("GET", base_uri + "/Movies/" + id);
  res = handler->handle_request(req);
  EXPECT_EQ(res->status_code, 404);

}

TEST_F(CrudRequestHandlerTestFixture, DeleteNonExistingEntity)
{
  std::string id = "1234";
  std::string entity_dir = test_dir + "/Movies";
  auto req = make_request("DELETE", base_uri + "/Movies/" + id);

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 404);
}

TEST_F(CrudRequestHandlerTestFixture, DeleteNoID)
{
  auto req = make_request("DELETE", base_uri + "/Movies");

  auto res = handler->handle_request(req);

  EXPECT_EQ(res->status_code, 405);
}