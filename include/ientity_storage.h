#ifndef IENTITY_STORAGE_H
#define IENTITY_STORAGE_H

#include <string>
#include <vector>
#include <optional>

class IEntityStorage
{
public:

    // Create an entity and return its ID
    // Takes in a resource (path) and data (JSON string)
    // Returns newly allocated ID of entity or nullopt otherwise
    virtual std::optional<int> create(const std::string &resource, const std::string &data) = 0;

    // Retriecve an entity by ID, given resource (path) and ID, return data (JSON string)
    // Returns the data as a JSON string or nullopt if not found
    virtual std::optional<std::string> retrieve(const std::string &resource, int id) const = 0;

    // Update an entity by ID, given resource (path), ID, and new data (JSON string)
    // Returns true if successful, false otherwise
    virtual bool update(const std::string &resource, int id, const std::string &data) = 0;

    // Remove an entity by ID, given resource (path) and ID
    // Returns true if successful, false otherwise
    virtual bool remove(const std::string &resource, int id) = 0;

    // List all entities of a given resource (path)
    // Returns a vector of IDs (integers) or an empty vector if none found
    // This is a JSON array of IDs
    virtual std::vector<int> list(const std::string &resource) const = 0;

    virtual ~IEntityStorage() = default;
};

#endif // IENTITY_STORAGE_H