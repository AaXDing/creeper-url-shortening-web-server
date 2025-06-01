#ifndef IDATABASE_CLIENT_H
#define IDATABASE_CLIENT_H

#include <optional>
#include <string>

///
/// Interface for a PostgreSQL‐style key→value store
///
class IDatabaseClient {
 public:
  virtual ~IDatabaseClient() = default;

  /// Upsert (short_code -> long_url). Return true on success, false on error.
  virtual bool store(const std::string& short_code,
                     const std::string& long_url) = 0;

  /// Return std::nullopt if not found; otherwise the stored long URL.
  virtual std::optional<std::string> lookup(const std::string& short_code) = 0;
};

#endif  // IDATABASE_CLIENT_H
