#ifndef SIM_ENTITY_STORAGE_H
#define SIM_ENTITY_STORAGE_H

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "ientity_storage.h"

class SimEntityStorage : public IEntityStorage {
 public:
  std::optional<int> create(const std::string &resource,
                            const std::string &data) override;

  std::optional<std::string> retrieve(const std::string &resource,
                                      int id) const override;

  bool update(const std::string &resource, int id,
              const std::string &data) override;

  bool remove(const std::string &resource, int id) override;

  std::vector<int> list(const std::string &resource) const override;

 private:
  // Simulated Storage: map resource paths to pairs of ID and data
  std::map<std::string, std::map<int, std::string>> storage;
  // Helper function to generate a new ID
  int get_next_available_id(const std::string &resource);
};

#endif