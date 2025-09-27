#include "shorten_request_handler.h"

#include <fstream>
#include "logging.h"
#include "real_database_client.h"
#include "real_redis_client.h"
#include "registry.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler,
                 ShortenRequestHandlerArgs);

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

bool validate_config_structure(
    std::shared_ptr<NginxConfigStatement> statement) {
  if (statement->child_block_->statements_.size() != 7) {
    return false;
  }

  const std::vector<std::string> expected_tokens = {
      "redis_ip", "redis_port", "db_host",  "db_name",
      "db_user",  "db_pass",    "pool_size"};

  for (size_t i = 0; i < expected_tokens.size(); i++) {
    if (statement->child_block_->statements_[i]->tokens_.size() != 2 ||
        statement->child_block_->statements_[i]->tokens_[0] !=
            expected_tokens[i]) {
      return false;
    }
  }

  return true;
}

std::shared_ptr<ShortenRequestHandlerArgs>
ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  auto args = std::make_shared<ShortenRequestHandlerArgs>();

  // If the environment variable is set, build tiny inline fakes here:
  if (std::getenv("USE_FAKE_SHORTEN_CLIENTS") != nullptr) {
    // A minimal "fake" that satisfies IRedisClient:
    struct FakeRedisLocal : IRedisClient {
      std::unordered_map<std::string, std::string> store_;
      std::optional<std::string> get(const std::string& short_code) override {
        auto it = store_.find(short_code);
        return (it == store_.end() ? std::nullopt
                                   : std::make_optional(it->second));
      }
      void set(const std::string& short_code,
               const std::string& long_url) override {
        store_[short_code] = long_url;
      }
    };

    // A minimal "fake" that satisfies IDatabaseClient:
    struct FakeDbLocal : IDatabaseClient {
      std::unordered_map<std::string, std::string> store_;
      bool store(const std::string& short_code,
                 const std::string& long_url) override {
        store_[short_code] = long_url;
        return true;
      }
      std::optional<std::string> lookup(
          const std::string& short_code) override {
        auto it = store_.find(short_code);
        return (it == store_.end() ? std::nullopt
                                   : std::make_optional(it->second));
      }
    };

    args->redis_client = std::make_shared<FakeRedisLocal>();
    args->db_client = std::make_shared<FakeDbLocal>();
    return args;
  }

  if (validate_config_structure(statement)) {
    std::string redis_ip = statement->child_block_->statements_[0]->tokens_[1];
    int redis_port =
        std::stoi(statement->child_block_->statements_[1]->tokens_[1]);
    std::string db_host = statement->child_block_->statements_[2]->tokens_[1];
    std::string db_name = statement->child_block_->statements_[3]->tokens_[1];
    std::string db_user = statement->child_block_->statements_[4]->tokens_[1];
    std::string db_pass = statement->child_block_->statements_[5]->tokens_[1];
    int pool_size =
        std::stoi(statement->child_block_->statements_[6]->tokens_[1]);

    // Construct concrete implementations and store them in args:
    args->redis_client =
        std::make_shared<RealRedisClient>(redis_ip, redis_port, pool_size);
    args->db_client = std::make_shared<RealDatabaseClient>(
        db_host, db_name, db_user, db_pass, pool_size);

    LOG(info) << "Finished creating shorten request handler args";

    return args;
  }

  return nullptr;
}

ShortenRequestHandler::ShortenRequestHandler(
    const std::string& base_uri,
    std::shared_ptr<ShortenRequestHandlerArgs> args)
    : base_uri_(base_uri), redis_(args->redis_client), db_(args->db_client) {
  if (!redis_) {
    LOG(fatal) << "ShortenRequestHandler requires a valid IRedisClient";
    exit(1);
  }
  if (!db_) {
    LOG(fatal) << "ShortenRequestHandler requires a valid IDatabaseClient";
    exit(1);
  }
}

ShortenRequestHandler::~ShortenRequestHandler() = default;

RequestHandler::HandlerType ShortenRequestHandler::get_type() const {
  return RequestHandler::HandlerType::SHORTEN_REQUEST_HANDLER;
}

std::unique_ptr<Response> ShortenRequestHandler::handle_request(
    const Request& request) {
  if (request.method == "POST") {
    return handle_post_request(request);
  } else if (request.method == "GET") {
    return handle_get_request(request);
  }

  auto res = std::make_unique<Response>();
  *res = STOCK_RESPONSE.at(405);
  return res;
}

// Long URL -> Short URL
std::unique_ptr<Response> ShortenRequestHandler::handle_post_request(
    const Request& request) {
  auto res = std::make_unique<Response>();

  std::string long_url = request.body;

  // Try to create a short code; on collision, compare and if needed, rehash with salt
  const int kMaxAttempts = 5;
  for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
    std::string salted_input = (attempt == 0)
                                   ? long_url
                                   : (long_url + "#" + std::to_string(attempt));
    std::string candidate_short = base62_encode(salted_input);

    // Check existing mapping for this candidate short code
    std::optional<std::string> existing = db_->lookup(candidate_short);
    if (existing.has_value()) {
      // If mapping already exists and matches the requested long URL, reuse it
      if (existing.value() == long_url) {
        res->status_code = 200;
        res->status_message = "OK";
        res->version = request.version;
        res->headers.push_back({"Content-Type", "text/plain"});
        res->body = candidate_short;
        return res;
      }
      // Collision with different long URL, try next salt
      continue;
    }

    // No existing mapping; store the original long URL under this short code
    if (!db_->store(candidate_short, long_url)) {
      LOG(error) << "Failed to store URL mapping: " << candidate_short << " -> "
                 << long_url;
      *res = STOCK_RESPONSE.at(500);
      res->body = "Failed to store URL mapping";
      return res;
    }

    res->status_code = 200;
    res->status_message = "OK";
    res->version = request.version;
    res->headers.push_back({"Content-Type", "text/plain"});
    res->body = candidate_short;
    return res;
  }

  // Exceeded max attempts to resolve collision
  LOG(error) << "Exceeded maximum attempts to resolve short URL collision";
  *res = STOCK_RESPONSE.at(500);
  res->body = "Could not generate unique short URL";
  return res;
}

// Short URL -> Long URL
std::unique_ptr<Response> ShortenRequestHandler::handle_get_request(
    const Request& request) {
  auto res = std::make_unique<Response>();

  // If the request is for the base /shorten path, serve the UI directly
  if (request.uri == base_uri_) {
    res->status_code = 200;
    res->status_message = "OK";
    res->version = request.version;
    res->headers.push_back({"Content-Type", "text/html"});
    
    // Read and serve the HTML file
    // std::ifstream file("/usr/src/projects/creeper/data/web/shorten.html"); // for local testing
    std::ifstream file("/data/web/shorten.html");  // Docker path
    if (file.is_open()) {
      std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
      res->body = content;
      file.close();
    } else {
      LOG(error) << "Failed to open shorten.html";
      *res = STOCK_RESPONSE.at(404);
    }
    return res;
  }

  // Must be /base_uri/6UQVxS
  if (request.uri.length() != base_uri_.length() + SHORT_URL_LENGTH + 1) {
    LOG(info) << "Invalid short URL: " << request.uri
              << " (expected short URL length: " << SHORT_URL_LENGTH << ")";
    *res = STOCK_RESPONSE.at(404);
    return res;
  }

  // /base_uri/6UQVxS --> 6UQVxS
  std::string short_url = request.uri.substr(base_uri_.length() + 1);

  // If Short URL is found in Redis, return 302
  std::optional<std::string> redis_long_url = redis_->get(short_url);
  if (redis_long_url) {
    LOG(info) << "Found in Redis: " << short_url << " -> "
              << redis_long_url.value();
    res->status_code = 302;
    res->status_message = "Found";
    res->version = request.version;
    // Set the location header to the long URL for redirection
    res->headers.push_back({"Location", redis_long_url.value()});
    return res;
  }

  // If Short URL is not found in Redis, check SQL database
  // std::optional<std::string> long_url = get_long_url(short_url);
  std::optional<std::string> long_url = db_->lookup(short_url);
  if (!long_url) {
    LOG(info) << "Not Found in DB: " << short_url;
    *res = STOCK_RESPONSE.at(404);
    return res;
  }

  // If Short URL is found in SQL database
  LOG(info) << "Found in DB: " << short_url << " -> " << long_url.value();

  // Store the short URL, long URL mapping in Redis
  redis_->set(short_url, long_url.value());

  res->status_code = 302;
  res->status_message = "Found";
  res->version = request.version;
  // Set the location header to the long URL for redirection
  res->headers.push_back({"Location", long_url.value()});
  return res;
}

std::string ShortenRequestHandler::base62_encode(const std::string& url) {
  std::string base62_url;
  std::hash<std::string> hasher;
  size_t num = hasher(url);

  // Convert the number to base62
  while (num > 0) {
    base62_url += BASE62_CHARS[num % 62];
    num /= 62;
  }

  // Restrict to 6 characters
  // If the encoded URL is less than 6 characters, pad with leading zeros
  while (base62_url.length() < SHORT_URL_LENGTH) {
    base62_url = "0" + base62_url;
  }
  // If the encoded URL is greater than 6 characters, truncate to 6 characters
  base62_url = base62_url.substr(0, SHORT_URL_LENGTH);

  return base62_url;
}
