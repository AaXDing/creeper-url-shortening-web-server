#ifndef FILE_ENTITY_STORAGE_H
#define FILE_ENTITY_STORAGE_H

#include "ientity_storage.h"

#include <mutex>
#include <optional>
#include <string>
#include <vector>

class FileEntityStorage : public IEntityStorage {
public:
  explicit FileEntityStorage(const std::string &root_path);
  std::optional<int> create(const std::string &resource,
                            const std::string &data) override;
  std::optional<std::string> retrieve(const std::string &resource,
                                      int id) const override;
  bool update(const std::string &resource, int id,
              const std::string &data) override;
  bool remove(const std::string &resource, int id) override;
  std::vector<int> list(const std::string &resource) const override;

  int get_next_available_id(const std::string &resource) const;

private:
  std::string make_entity_path(const std::string &resource, int id) const;
  std::string make_entity_dir(const std::string &resource) const;
  std::string root_;
};

#endif // FILE_ENTITY_STORAGE_H
