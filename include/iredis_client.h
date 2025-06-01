#ifndef IREDIS_CLIENT_H
#define IREDIS_CLIENT_H

#include <optional>
#include <string>

///
/// Interface for a Redis‐style cache
///
class IRedisClient {
 public:
  virtual ~IRedisClient() = default;

  /// Return std::nullopt if not found; otherwise the stored long URL.
  virtual std::optional<std::string> get(const std::string& short_code) = 0;

  /// Store (short_code -> long_url); no return value.
  virtual void set(const std::string& short_code,
                   const std::string& long_url) = 0;
};

#endif  // IREDIS_CLIENT_H
