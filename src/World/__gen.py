# Создаем все файлы системы
files = {
    "WComponent.h": """#pragma once
#include "WObject.h"

class WComponent : public WObject {
public:
    WComponent() = delete;
    WComponent(WObject* owner, std::string name = "Component");
    bool IsComponent() const override { return true; }
    
    void AddChild(Ref<WObject> child) override {
        throw std::logic_error("Components cannot have children");
    }
    
protected:
    void InitializeAsComponent(WObject* owner);
};

DECLARE_DATA_TYPE(WComponent);""",

    "WComponent.cpp": """#include "WComponent.h"
#include "WObject.h"

WComponent::WComponent(WObject* owner, std::string name)
    : WObject(std::move(name)) 
{
    if (!owner) {
        throw std::invalid_argument("Component must have an owner");
    }
    InitializeAsComponent(owner);
}

void WComponent::InitializeAsComponent(WObject* owner) {
    owner->AddComponent(Ref<WComponent>(this));
}""",

    "WActor.h": """#pragma once
#include "WObject.h"

class WActor : public WObject {
public:
    WActor(std::string name = "Actor");
    bool CanBeComponent() const override { return false; }
};

DECLARE_DATA_TYPE(WActor);""",

    "WActor.cpp": """#include "WActor.h"

WActor::WActor(std::string name)
    : WObject(std::move(name)) {}""",

    "PlayerCharacter.h": """#pragma once
#include "WActor.h"

class PlayerCharacter : public WActor {
public:
    PlayerCharacter(std::string name = "Player");
    void Update(float delta_time) override;
    float health = 100.0f;
};

DECLARE_DATA_TYPE(PlayerCharacter);""",

    "PlayerCharacter.cpp": """#include "PlayerCharacter.h"

PlayerCharacter::PlayerCharacter(std::string name)
    : WActor(std::move(name)) {}

void PlayerCharacter::Update(float delta_time) {
    WActor::Update(delta_time);
    // Game character logic
}"""
}

# Создаем обновленные версии WObject.h и WObject.cpp
files["WObject.h"] = """#pragma once
#include "../core/core.h"
#include <vector>
#include <unordered_set>
#include <string>
#include <stdexcept>
#include <algorithm>

class World;
class Camera;
class Light;

class WObject {
public:
    WObject(std::string name = "Object");
    virtual ~WObject() = default;
    
    virtual bool IsComponent() const { return owner != nullptr; }
    virtual bool CanBeComponent() const { return true; }
    bool IsAttached() const { return is_attached; }
    WObject* GetOwner() const { return owner; }
    
    void AddChild(Ref<WObject> child);
    void RemoveChild(Ref<WObject> child);
    const std::vector<Ref<WObject>>& GetChildren() const { return children; }
    
    void AddComponent(Ref<WObject> component);
    void RemoveComponent(Ref<WObject> component);
    std::vector<Ref<WObject>> GetComponents() const { return components; }
    
    template<typename T>
    std::vector<Ref<T>> GetComponentsOfType() {
        std::vector<Ref<T>> result;
        for (auto& comp : components) {
            if (auto casted = RefCast<T>(comp)) {
                result.push_back(casted);
            }
            std::vector<Ref<T>> t = comp->GetComponentsOfType<T>();
            if(t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }

    template<typename T>
    std::vector<Ref<T>> GetChildsOfType() {
        std::vector<Ref<T>> result;
        for (auto& child : children) {
            if (auto casted = RefCast<T>(child)) {
                result.push_back(casted);
            }
            std::vector<Ref<T>> t = child->GetChildsOfType<T>();
            if(t.size())
                result.insert(result.end(), t.begin(), t.end());
        }
        return result;
    }
    
    void SetPosition(MVector3f position);
    void SetRotation(MVector3f euler_angles);
    void SetRotation(MVector4f quaternion);
    void SetScale(MVector3f scale);
    MVector3f GetPosition() const;
    MVector4f GetRotation() const;
    MVector3f GetScale() const;
    MMatrix4f GetWorldTransform() const;
    MMatrix4f GetLocalTransform() const { return local_transform; }
    
    void AddTag(std::string tag);
    void RemoveTag(std::string tag);
    bool HasTag(std::string tag) const;
    const std::unordered_set<std::string>& GetTags() const { return tags; }
    
    virtual void Update(float delta_time);
    
    std::string name;
    World* world = nullptr;
    
protected:
    MMatrix4f local_transform = MMatrix4f::identity();
    std::vector<Ref<WObject>> children;
    std::vector<Ref<WObject>> components;
    std::unordered_set<std::string> tags;
    WObject* owner = nullptr;
    bool is_attached = false;
    
    void AttachTo(WObject* new_owner);
    void Detach();
    bool IsAttachedTo(WObject* potential_owner) const;
    
    void ValidateComponentAddition(Ref<WObject> component) {
        if (!component || component->IsComponent() || !component->CanBeComponent()) {
            throw std::invalid_argument("Object cannot be added as component");
        }
    }
};

DECLARE_DATA_TYPE(WObject);"""

files["WObject.cpp"] = """#include "WObject.h"
#include "World.h"
#include <stdexcept>

WObject::WObject(std::string name) : name(std::move(name)) {
    local_transform = MMatrix4f::identity();
}

void WObject::AddChild(Ref<WObject> child) {
    if (!child || child == this) {
        return;
    }
    
    if (child->IsAttachedTo(this)) {
        throw std::logic_error("Cannot create circular dependency");
    }
    
    if (child->owner) {
        child->Detach();
    }
    
    children.push_back(child);
    child->AttachTo(this);
    
    if (world && !child->world) {
        world->AddObject(child);
    }
}

void WObject::RemoveChild(Ref<WObject> child) {
    auto it = std::find(children.begin(), children.end(), child);
    if (it != children.end()) {
        (*it)->Detach();
        children.erase(it);
    }
}

void WObject::AddComponent(Ref<WObject> component) {
    ValidateComponentAddition(component);
    
    if (component->owner) {
        throw std::logic_error("Object is already a component");
    }
    
    if (component->IsAttachedTo(this)) {
        throw std::logic_error("Cannot create circular dependency");
    }
    
    components.push_back(component);
    component->AttachTo(this);
}

void WObject::RemoveComponent(Ref<WObject> component) {
    auto it = std::find(components.begin(), components.end(), component);
    if (it != components.end()) {
        (*it)->Detach();
        components.erase(it);
    }
}

void WObject::AttachTo(WObject* new_owner) {
    if (owner || is_attached) {
        throw std::logic_error("Object is already attached");
    }
    
    owner = new_owner;
    is_attached = true;
    world = owner->world;
}

void WObject::Detach() {
    owner = nullptr;
    is_attached = false;
}

bool WObject::IsAttachedTo(WObject* potential_owner) const {
    if (!potential_owner) return false;
    
    const WObject* current = this;
    while (current) {
        if (current == potential_owner) return true;
        current = current->owner;
    }
    return false;
}

MVector3f WObject::GetPosition() const {
    return wm::GetTranslation(local_transform);
}

MVector4f WObject::GetRotation() const {
    return wm::GetRotation(local_transform);
}

MVector3f WObject::GetScale() const {
    return wm::GetScale(local_transform);
}

MMatrix4f WObject::GetWorldTransform() const {
    if (owner) {
        return owner->GetWorldTransform() * local_transform;
    }
    return local_transform;
}

void WObject::SetPosition(MVector3f position) {
    wm::SetTranslation(local_transform, position);
}

void WObject::SetRotation(MVector3f rotation) {
    wm::SetRotation(local_transform, rotation);
}

void WObject::SetRotation(MVector4f quaternion) {
    wm::SetRotation(local_transform, quaternion);
}

void WObject::SetScale(MVector3f scale) {
    wm::SetScale(local_transform, scale);
}

void WObject::AddTag(std::string tag) {
    tags.insert(tag);
    if (world) world->RegisterTags(Ref<WObject>(this));
}

void WObject::RemoveTag(std::string tag) {
    tags.erase(tag);
    if (world) world->UnregisterTags(Ref<WObject>(this));
}

bool WObject::HasTag(std::string tag) const {
    return tags.find(tag) != tags.end();
}

void WObject::Update(float delta_time) {
    for (auto& comp : components) {
        comp->Update(delta_time);
    }
    
    for (auto& child : children) {
        child->Update(delta_time);
    }
}"""

# Генерируем все файлы
for filename, content in files.items():
    with open(filename, "w") as f:
        f.write(content)

print("Все файлы успешно созданы!")