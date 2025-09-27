#include "World.h"
#include "WObject.h"

World::World(std::string name) : name(std::move(name)) {}

void World::AddObject(Ref<WObject> object) {
    if (!object || object->world) return;
    
    object->world = this;
    if (!object->GetOwner()) {
        root_objects.push_back(object);
    }
    RegisterTags(object);
}

void World::RemoveObject(Ref<WObject> object) {
    if (!object || object->world != this) return;
    
    UnregisterTags(object);
    object->world = nullptr;
    
    if (!object->GetOwner()) {
        auto it = std::remove(root_objects.begin(), root_objects.end(), object);
        root_objects.erase(it, root_objects.end());
    }
}

std::vector<Ref<WObject>> World::FindObjectsByTag(std::string tag) {
    if (auto it = tag_registry.find(tag); it != tag_registry.end()) {
        return it->second;
    }
    return {};
}

void World::Update(float delta_time) {
    for (auto& object : root_objects) {
        object->Update(delta_time);
    }
}

void World::RegisterTags(Ref<WObject> object) {
    for (const auto& tag : object->GetTags()) {
        tag_registry[tag].push_back(object);
    }
}

void World::UnregisterTags(Ref<WObject> object) {
    for (const auto& tag : object->GetTags()) {
        auto& objects = tag_registry[tag];
        auto it = std::remove(objects.begin(), objects.end(), object);
        objects.erase(it, objects.end());
    }
}