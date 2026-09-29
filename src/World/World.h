#pragma once
#include "../core/core.h"
#include <unordered_map>
#include <vector>
#include <algorithm>

class WObject;
class Light;
class Camera;

class World : public CoreObject {
public:
    World(std::string name = "World");
    
    void AddObject(Ref<WObject> object);
    void RemoveObject(Ref<WObject> object);
    
    std::vector<Ref<WObject>> FindObjectsByTag(std::string tag);
    void Update(float delta_time);
    
    std::string GetName() const { return name; }
    
    void RegisterTags(Ref<WObject> object);
    void UnregisterTags(Ref<WObject> object);
    
    const std::list<Ref<WObject>>& GetObjects() const { return root_objects; }
    const std::list<Ref<Light>>&   GetLights() const  { return lights; }
    Ref<Camera>                    GetDefaultCamera() const  { return default_camera; }
    void                           SetDefaultCamera(Ref<Camera> value)  { default_camera = value; }

private:
    std::string name;
    SpinLocker root_objects_lock;
    std::list<Ref<WObject>> root_objects;
    SpinLocker new_root_objects_lock;
    std::list<Ref<WObject>> new_root_objects;
    SpinLocker rem_root_objects_lock;
    std::list<Ref<WObject>> rem_root_objects;
    std::list<Ref<Light>>   lights;
    Ref<Camera>             default_camera;

    constexpr static uint16_t pool_count = 256;
    std::atomic_uint8_t     pool_status;
    std::vector<Ref<WObject>> pools[pool_count];

    std::unordered_map<std::string, std::vector<Ref<WObject>>> tag_registry;

    float accumulated_delta_time = 0;
    
};
