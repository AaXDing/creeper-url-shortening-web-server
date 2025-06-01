#include "real_database_client.h"

RealDatabaseClient::RealDatabaseClient(const std::string& db_host,
                                       const std::string& db_name,
                                       const std::string& db_user,
                                       const std::string& db_password) {
  std::string conninfo = "host=" + db_host + " dbname=" + db_name +
                         " user=" + db_user + " password=" + db_password;

  conn_ = PQconnectdb(conninfo.c_str());
  if (PQstatus(conn_) != CONNECTION_OK) {
    std::string err = PQerrorMessage(conn_);
    LOG(fatal) << "Failed to connect to Postgres at " << db_host
               << " (db=" << db_name << ", user=" << db_user << "): " << err;
    PQfinish(conn_);
    exit(1);
  }
}

RealDatabaseClient::~RealDatabaseClient() {
  if (conn_) {
    PQfinish(conn_);
  }
}

bool RealDatabaseClient::store(const std::string& short_code,
                               const std::string& long_url) {
  const int nParams = 2;
  const char* paramValues[nParams] = {short_code.c_str(), long_url.c_str()};
  const int paramLengths[nParams] = {static_cast<int>(short_code.size()),
                                     static_cast<int>(long_url.size())};
  const int paramFormats[nParams] = {0, 0};  // text format

  const char* query =
      "INSERT INTO short_to_long_url (short_url, long_url) "
      "VALUES ($1, $2) "
      "ON CONFLICT (short_url) DO UPDATE SET long_url = EXCLUDED.long_url";

  PGresult* res = PQexecParams(conn_, query, nParams,
                               nullptr,  // let libpq infer parameter types
                               paramValues, paramLengths, paramFormats,
                               0  // request text results
  );

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    std::string err = PQerrorMessage(conn_);
    PQclear(res);
    LOG(fatal) << "Postgres STORE error (key=" << short_code << "): " << err;
    exit(1);
  }

  PQclear(res);
  return true;
}

std::optional<std::string> RealDatabaseClient::lookup(
    const std::string& short_code) {
  const int nParams = 1;
  const char* paramValues[nParams] = {short_code.c_str()};
  const int paramLengths[nParams] = {static_cast<int>(short_code.size())};
  const int paramFormats[nParams] = {0};  // text format

  const char* query =
      "SELECT long_url FROM short_to_long_url WHERE short_url = $1";

  PGresult* res = PQexecParams(conn_, query, nParams, nullptr, paramValues,
                               paramLengths, paramFormats,
                               0  // request text results
  );

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    std::string err = PQerrorMessage(conn_);
    PQclear(res);
    LOG(fatal) << "Postgres LOOKUP error (key=" << short_code << "): " << err;
    exit(1);
  }

  std::optional<std::string> result;
  if (PQntuples(res) > 0) {
    result = PQgetvalue(res, 0, 0);
  }
  PQclear(res);
  return result;
}
