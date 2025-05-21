#include "sim_entity_storage.h"
#include "logging.h"

int SimEntityStorage::generateId(const std::string &resource)
{
    //find the largest ID in the resource path and return that ID + 1
    //search for the resource path in storage
    auto it = storage.find(resource);
    // If the resource path does not exist, return 1
    if (it == storage.end())
    {
        return 1;
    }
    // If the resource path exists, search for largest used ID
    int maxID = 0;
    for (const auto &entity : it->second)
    {
        if (entity.first > maxID)
        {
            maxID = entity.first;
        }
    }
    // Return the next available ID
    if (maxID >= INT_MAX)
    {
        LOG(error) << "Exceeded maximum ID value: cannot assign new ID";
        return -1; // or throw, or handle differently if needed
    }
    return maxID + 1;
}

std::optional<int> SimEntityStorage::create(const std::string &resource, const std::string &data)
{
    // check of resource path exists in storage
    int nextid = generateId(resource);
    auto it = storage.find(resource);
    // If the resource path does not exist, create a new resource
    if (it == storage.end())
    {
        LOG(debug) << "Creating new resource: " << resource;
        std::map<int, std::string> entity;
        entity[nextid] = data;
        storage[resource] = entity;
        LOG(info) << "Created new resource: " << resource;
        LOG(info) << "Creating new entity " << resource << "/" << std::to_string(nextid);
        return nextid;
    }

    // Add data at id
    storage[resource][nextid] = data;
    LOG(info) << "Creating new entity " << resource << "/" << std::to_string(nextid);
    LOG(debug) << "Creating new entity " << resource << "/" << std::to_string(nextid);

    return nextid;
}

std::optional<std::string> SimEntityStorage::retrieve(const std::string &resource, int id) const
{
    LOG(info) << "Retrieving " << resource << "/" << std::to_string(id);
    auto it = storage.find(resource);
    if (it != storage.end())
    {
        auto entityIt = it->second.find(id);
        if (entityIt != it->second.end())
        {
            return entityIt->second;
        }
    }
    LOG(warning) << "Entity not found";
    return std::nullopt;
}

bool SimEntityStorage::update(const std::string &resource, int id, const std::string &data)
{
    auto it = storage.find(resource);
    if (it != storage.end())
    {
        auto entityIt = it->second.find(id);
        if (entityIt != it->second.end())
        {
            LOG(info) << "Updating " << resource << "/" << std::to_string(id);
            entityIt->second = data;
            return true;
        }
        else //resource path exists but id not found
        {
            // create a new entity
            LOG(info) << "Creating new entity " << resource << "/" << std::to_string(id);
            storage[resource][id] = data;
            return true;
        }

    }
    else //needs a new resource path
    {
        LOG(info) << "Creating new resource: " << resource;
        std::map<int, std::string> entity;
        entity[id] = data;
        storage[resource] = entity;
        LOG(info) << "Creating new entity " << resource << "/" << std::to_string(id);
        return true;
    }
    return false;
}

bool SimEntityStorage::remove(const std::string &resource, int id)
{
    auto it = storage.find(resource);
    if (it != storage.end())
    {
        auto entityIt = it->second.find(id);
        if (entityIt != it->second.end())
        {
            it->second.erase(entityIt);
            LOG(info) << "Removing " << resource << "/" << std::to_string(id);
            return true;
        }
    }
    LOG(warning) << "Entity not found";
    return false;
}
std::vector<int> SimEntityStorage::list(const std::string &resource) const
{
    std::vector<int> ids;
    auto it = storage.find(resource);
    if (it != storage.end())
    {
        for (const auto &entity : it->second)
        {
            ids.push_back(entity.first);
        }
    }
    return ids;
}