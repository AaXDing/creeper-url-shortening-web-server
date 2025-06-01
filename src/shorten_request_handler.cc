#include "shorten_request_handler.h"

#include "logging.h"
#include "real_database_client.h"
#include "real_redis_client.h"
#include "registry.h"

REGISTER_HANDLER("ShortenHandler", ShortenRequestHandler,
                 ShortenRequestHandlerArgs);

ShortenRequestHandlerArgs::ShortenRequestHandlerArgs() {}

std::shared_ptr<ShortenRequestHandlerArgs>
ShortenRequestHandlerArgs::create_from_config(
    std::shared_ptr<NginxConfigStatement> statement) {
  return std::make_shared<ShortenRequestHandlerArgs>();
}

ShortenRequestHandler::ShortenRequestHandler(
    const std::string& base_uri,
    std::shared_ptr<ShortenRequestHandlerArgs> args)
    : base_uri_(base_uri), pg_conn_(nullptr) {
  // Try to connect to Redis
  try {
    std::string redis_uri =
        "tcp://" + REDIS_IP + ":" + std::to_string(REDIS_PORT);
    redis_ = std::make_shared<Redis>(redis_uri);
  } catch (const Error& e) {
    LOG(fatal) << "Failed to connect to Redis: " << e.what();
    exit(1);
  }

  // Initialize PostgreSQL connection
  if (!init_db()) {
    LOG(fatal) << "Failed to initialize PostgreSQL database";
    exit(1);
  }
}

ShortenRequestHandler::~ShortenRequestHandler() {
  if (pg_conn_) {
    PQfinish(pg_conn_);
  }
}

bool ShortenRequestHandler::init_db() {
  std::string conninfo = "host=" + DB_HOST + " dbname=" + DB_NAME +
                         " user=" + DB_USER + " password=" + DB_PASSWORD;
  pg_conn_ = PQconnectdb(conninfo.c_str());

  if (PQstatus(pg_conn_) != CONNECTION_OK) {
    LOG(error) << "Connection to database failed: " << PQerrorMessage(pg_conn_);
    return false;
  }

  return true;
}

bool ShortenRequestHandler::store_url_mapping(const std::string& short_url,
                                              const std::string& long_url) {
  const int param_count = 2;
  const char* param_values[param_count] = {short_url.c_str(), long_url.c_str()};
  const int param_lengths[param_count] = {static_cast<int>(short_url.length()),
                                          static_cast<int>(long_url.length())};
  const int param_formats[param_count] = {0, 0};  // 0 = text format

  // Insert the short URL and long URL into the database
  PGresult* res = PQexecParams(
      pg_conn_,
      "INSERT INTO short_to_long_url (short_url, long_url) VALUES ($1, $2) ON "
      "CONFLICT(short_url) DO UPDATE SET long_url = $2 ",
      param_count, nullptr, param_values, param_lengths, param_formats, 0);

  // If we do not want to update the long URL if the short URL already exists,
  // use the following query:
  // "INSERT INTO short_to_long_url (short_url, long_url) VALUES ($1, $2)"

  bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
  if (!success) {
    LOG(error) << "Failed to store URL mapping: " << PQerrorMessage(pg_conn_);
  }
  PQclear(res);
  return success;
}

std::optional<std::string> ShortenRequestHandler::get_long_url(
    const std::string& short_url) {
  const int param_count = 1;
  const char* param_values[param_count] = {short_url.c_str()};
  const int param_lengths[param_count] = {static_cast<int>(short_url.length())};
  const int param_formats[param_count] = {0};  // 0 = text format

  // SELECT the long URL from the database using the short URL
  PGresult* res = PQexecParams(
      pg_conn_, "SELECT long_url FROM short_to_long_url WHERE short_url = $1",
      param_count, nullptr, param_values, param_lengths, param_formats, 0);

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    LOG(error) << "Failed to get long URL: " << PQerrorMessage(pg_conn_);
    PQclear(res);
    return std::nullopt;
  }

  std::optional<std::string> result;
  if (PQntuples(res) > 0) {
    result = PQgetvalue(res, 0, 0);
  }
  PQclear(res);
  return result;
}

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
  std::string short_url = base62_encode(long_url);

  // Store the mapping in PostgreSQL
  if (!store_url_mapping(short_url, long_url)) {
    LOG(error) << "Failed to store URL mapping: " << short_url << " -> "
               << long_url;
    *res = STOCK_RESPONSE.at(500);
    res->body = "Failed to store URL mapping";
    return res;
  }

  res->status_code = 200;
  res->status_message = "OK";
  res->version = request.version;
  res->headers.push_back({"Content-Type", "text/plain"});
  res->body = short_url;
  return res;
}

// Short URL -> Long URL
std::unique_ptr<Response> ShortenRequestHandler::handle_get_request(
    const Request& request) {
  auto res = std::make_unique<Response>();

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
  std::optional<std::string> long_url = get_long_url(short_url);
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
