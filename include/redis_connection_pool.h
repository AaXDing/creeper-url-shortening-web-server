#ifndef REDIS_CONNECTION_POOL_H
#define REDIS_CONNECTION_POOL_H

#include <sw/redis++/redis++.h>

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

// Redis connection pool
class RedisConnectionPool {
 public:
  RedisConnectionPool(const std::string& redis_ip, int redis_port,
                      size_t pool_size = 12);
  ~RedisConnectionPool();

  // Get a connection from the pool
  std::shared_ptr<sw::redis::Redis> acquire();
  // Return a connection to the pool
  void release(std::shared_ptr<sw::redis::Redis> conn);

 private:
  std::string connection_string_;
  size_t pool_size_;
  std::queue<std::shared_ptr<sw::redis::Redis>> connections_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

#endif  // REDIS_CONNECTION_POOL_H