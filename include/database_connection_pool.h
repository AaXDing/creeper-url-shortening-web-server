#ifndef DATABASE_CONNECTION_POOL_H
#define DATABASE_CONNECTION_POOL_H

#include <libpq-fe.h>

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

// PostgreSQL connection pool
class PostgresConnectionPool {
 public:
  PostgresConnectionPool(const std::string& db_host, const std::string& db_name,
                         const std::string& db_user,
                         const std::string& db_password, size_t pool_size = 12);
  ~PostgresConnectionPool();

  // Get a connection from the pool
  PGconn* acquire();
  // Return a connection to the pool
  void release(PGconn* conn);

 private:
  std::string connection_string_;
  size_t pool_size_;
  std::queue<PGconn*> connections_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

#endif  // DATABASE_CONNECTION_POOL_H