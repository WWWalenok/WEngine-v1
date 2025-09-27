#pragma once
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
    bool IsChild() const { return parent != nullptr; }
    WObject* GetOwner() const { return owner; }
    WObject* GetParent() const { return parent; }
    
    
    virtual void AddChild(Ref<WObject> child);
    virtual void RemoveChild(Ref<WObject> child);
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
    WObject* parent = nullptr;
    
    virtual void AttachTo(WObject* new_owner);
    virtual void SetParent(WObject* new_parent);
    void Detach();
    bool IsAttachedTo(WObject* object) const { return IsAttachedToOwner(object) || IsAttachedToParent(object); };
    bool IsAttachedToOwner(WObject* potential_owner) const;
    bool IsAttachedToParent(WObject* potential_parent) const;

};

DECLARE_DATA_TYPE(WObject);