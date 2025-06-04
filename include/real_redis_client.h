#ifndef REAL_REDIS_CLIENT_H
#define REAL_REDIS_CLIENT_H

#include <sw/redis++/redis++.h>

#include <memory>
#include <optional>
#include <string>

#include "iredis_client.h"
#include "logging.h"
#include "redis_connection_pool.h"

using namespace sw::redis;

///
/// RealRedisClient wraps sw::redis::Redis and implements IRedisClient.
/// On any connection‐failure or SET‐failure, it logs a fatal error and calls
/// exit(1).
///
class RealRedisClient : public IRedisClient {
 public:
  // Constructor: takes Redis IP (e.g. "127.0.0.1") and port (e.g. 6379).
  // Attempts a ping immediately; on failure, logs and exits.
  RealRedisClient(const std::string& redis_ip, int redis_port);

  ~RealRedisClient() override = default;

  // Return std::nullopt if not found or on GET error.
  std::optional<std::string> get(const std::string& short_code) override;

  // Store (short_code -> long_url). On SET error, logs and exits.
  void set(const std::string& short_code, const std::string& long_url) override;

 private:
  std::shared_ptr<RedisConnectionPool> pool_;
};

#endif  // REAL_REDIS_CLIENT_H
