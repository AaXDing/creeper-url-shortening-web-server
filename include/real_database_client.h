#ifndef REAL_DATABASE_CLIENT_H
#define REAL_DATABASE_CLIENT_H

#include <libpq-fe.h>
#include <memory>
#include <optional>
#include <string>

#include "database_connection_pool.h"
#include "idatabase_client.h"
#include "logging.h"

///
/// RealDatabaseClient wraps libpq calls (PQconnectdb, PQexecParams, etc.)
/// Implements IDatabaseClient. On any connection‐failure or SQL error, logs
/// a fatal error and calls exit(1).
///
class RealDatabaseClient : public IDatabaseClient {
 public:
  // Constructor: takes host, dbname, user, and password. On failure, logs and
  // exits.
  RealDatabaseClient(const std::string& db_host, const std::string& db_name,
                     const std::string& db_user,
                     const std::string& db_password);

  ~RealDatabaseClient() override = default;

  // Upsert (short_code -> long_url). On failure, logs and exits.
  bool store(const std::string& short_code,
             const std::string& long_url) override;

  // Lookup (short_code). Returns std::nullopt if not found. On SQL error, logs
  // and exits.
  std::optional<std::string> lookup(const std::string& short_code) override;

 private:
  std::shared_ptr<PostgresConnectionPool> pool_;
};

#endif  // REAL_DATABASE_CLIENT_H
