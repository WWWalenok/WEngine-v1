#include "World.h"
#include "WObject.h"
#include "Light.h"
#include "../game/GameSystem.h"

World::World(std::string name) : name(std::move(name)) {
    pool_status.store(0);
}

void World::AddObject(Ref<WObject> object) {
    if (!object || object->world) return;
    auto light = RefCast<Light>(object);
    if (light)
    {
        lights.push_back(light);
        return;
    }
    
    object->world = GetSelfRef(this);
    if (!object->GetOwner()) {
        auto lg = new_root_objects_lock.lock_guard();
        new_root_objects.push_back(object);
    }
    RegisterTags(object);
}

void World::RemoveObject(Ref<WObject> object) 
{
    if (!object || object->world != this) 
    {
        return;
    }
    
    UnregisterTags(object);
    object->world = nullptr;
    
    if (!object->GetOwner()) 
    {
        auto lg = rem_root_objects_lock.lock_guard();
        rem_root_objects.push_back(object);
    }
}

std::vector<Ref<WObject>> World::FindObjectsByTag(std::string tag) {
    if (auto it = tag_registry.find(tag); it != tag_registry.end()) {
        return it->second;
    }
    return {};
}

void World::Update(float delta_time) {
    WL_START_TYMETRACE;
    accumulated_delta_time += delta_time;
    if (pool_status.load() != 0)
        return;
    pool_status.store(pool_count);
    
    new_root_objects_lock.lock();
    std::list<Ref<WObject>> loc_new_root_objects;
    new_root_objects.swap(loc_new_root_objects);
    new_root_objects_lock.unlock();

    rem_root_objects_lock.lock();
    std::list<Ref<WObject>> loc_rem_root_objects;
    rem_root_objects.swap(loc_rem_root_objects);
    rem_root_objects_lock.lock();
    if(loc_new_root_objects.size() > 0 || loc_rem_root_objects.size() > 0)
    {
        auto lg = root_objects_lock.lock_guard();
        for (auto& object : loc_rem_root_objects) {
            auto it = std::remove(root_objects.begin(), root_objects.end(), object);
            root_objects.erase(it, root_objects.end());
        }
        for (auto& object : loc_new_root_objects) {
            root_objects.push_back(object);
        }

        size_t pool_size = root_objects.size() / pool_count + 1;
        for (uint16_t i = 0; i < pool_count; ++i)
        {
            if(pools[i].size() < pool_size)
                pools[i].reserve(pool_size);
            pools[i].clear();
        }
        uint16_t j = 0;
        for (auto& object : root_objects) {
            if(object->can_tick)
            {
                pools[j].push_back(object);
                j = (j + 1) % pool_count;
            }
        }
    }
    auto gs = GameSystem::Get();
    for (uint16_t i = 0; i < pool_count; ++i)
    {
        if(pools[i].size() > 0)
        {
            gs->Pool().send([delta_time = accumulated_delta_time, pool = &pools[i], Self = GetSelfRef(this)] () mutable {
                WL_START_TYMETRACE;
                for (auto& object : (*pool)) {
                    object->UpdateImp(delta_time);
                }
                Self->pool_status.fetch_sub(1);
            });
        }
    }
    accumulated_delta_time = 0;
}

void World::RegisterTags(Ref<WObject> object) {
    for (const auto& tag : object->GetTags()) {
        tag_registry[tag.first].push_back(object);
    }
}

void World::UnregisterTags(Ref<WObject> object) {
    for (const auto& tag : object->GetTags()) {
        auto& objects = tag_registry[tag.first];
        auto it = std::remove(objects.begin(), objects.end(), object);
        objects.erase(it, objects.end());
    }
}