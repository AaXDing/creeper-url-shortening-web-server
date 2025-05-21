#include "real_entity_storage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "logging.h"

namespace fs = std::filesystem;

RealEntityStorage::RealEntityStorage(const std::string &root_path)
    : root_(root_path) {
  fs::create_directories(root_);
}

// Generate next available integer ID by checking existing filenames
int RealEntityStorage::get_next_available_id(
    const std::string &entity_dir) const {
  int max_id = 0;
  for (const auto &entry : std::filesystem::directory_iterator(entity_dir)) {
    try {
      int id = std::stoi(entry.path().filename().string());
      if (id > max_id) max_id = id;
    } catch (...) {
      continue;  // skip files that can't be parsed as integers
    }
  }

  if (max_id >= INT_MAX) {
    LOG(error) << "Exceeded maximum ID value: cannot assign new ID";
    return -1;  // or throw, or handle differently if needed
  }

  return max_id + 1;
}

// Returns the full directory path for a specific entity
std::string RealEntityStorage::make_entity_dir(
    const std::string &resource) const {
  return (fs::path(root_) / resource).string();
}

// Returns the full path for a specific entity and ID
std::string RealEntityStorage::make_entity_path(const std::string &resource,
                                                int id) const {
  return (fs::path(root_) / resource / std::to_string(id)).string();
}

// Create: a new entity assign lowest unused ID, write data to file, return new
// ID
std::optional<int> RealEntityStorage::create(const std::string &resource,
                                             const std::string &data) {
  const std::string entity_dir = make_entity_dir(resource);
  fs::create_directories(entity_dir);  // Ensure entity directory exists

  int id = get_next_available_id(entity_dir);
  if (id < 0) return std::nullopt;

  std::ofstream file(make_entity_path(resource, id));
  if (!file) return std::nullopt;

  file << data;
  return id;
}

// Retrieve: the content for a given entity and ID
std::optional<std::string> RealEntityStorage::retrieve(
    const std::string &resource, int id) const {
  fs::path filepath = make_entity_path(resource, id);
  std::ifstream file(filepath);
  if (!file) return std::nullopt;

  std::ostringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

// Update or create: an entity at a specific ID with new data, returns true on
// success
bool RealEntityStorage::update(const std::string &resource, int id,
                               const std::string &data) {
  fs::create_directories(make_entity_dir(resource));

  fs::path filepath = make_entity_path(resource, id);
  std::ofstream file(filepath);  // truncates or creates
  if (!file) return false;

  file << data;
  return true;
}

// Delete: the file associated with the given entity and ID, returns true on
// success
bool RealEntityStorage::remove(const std::string &resource, int id) {
  fs::path filepath = make_entity_path(resource, id);
  return fs::remove(filepath);  // is false if no file was removed.
}

// List: all valid integer IDs for the given entity type by enumerating
// filenames
std::vector<int> RealEntityStorage::list(const std::string &resource) const {
  std::vector<int> ids;
  fs::path entity_dir = make_entity_dir(resource);

  if (!fs::exists(entity_dir) || !fs::is_directory(entity_dir)) {
    return ids;
  }

  for (const auto &entry : fs::directory_iterator(entity_dir)) {
    if (!entry.is_regular_file()) continue;

    const auto filename = entry.path().filename().string();
    try {
      ids.push_back(std::stoi(filename));
    } catch (const std::exception &) {
      // Skip non-integer filenames
    }
  }

  return ids;
}
