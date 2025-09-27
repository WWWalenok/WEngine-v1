#pragma once
#include "../core/core.h"
#include "WObject.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

class World {
public:
    World(std::string name = "World");
    
    void AddObject(Ref<WObject> object);
    void RemoveObject(Ref<WObject> object);
    
    std::vector<Ref<WObject>> FindObjectsByTag(std::string tag);
    void Update(float delta_time);
    
    std::string GetName() const { return name; }
    
    void RegisterTags(Ref<WObject> object);
    void UnregisterTags(Ref<WObject> object);
    
    const std::vector<Ref<WObject>>& GetObjects() const { return root_objects; }

private:
    std::string name;
    std::vector<Ref<WObject>> root_objects;
    std::unordered_map<std::string, std::vector<Ref<WObject>>> tag_registry;
    
};

DECLARE_DATA_TYPE(World);