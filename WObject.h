#include "WObject.h.imp.hpp"
#include "WObject.h.imp.hpp"

#ifndef WOBJECT_H_A15462527E7955E7_GUARD
#define WOBJECT_H_A15462527E7955E7_GUARD


#include "../core/core.h"
#include <unordered_map>

struct World;

WENGINE_CLASS_TAGS(awdawd);
class WObject {
public:
    WObject(const std::string& name = "Object");
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
    const std::vector<Ref<WObject>>& GetComponents() const { return components; }
    
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
    
    void AddTag(std::string tag, Data value = nullptr);
    void RemoveTag(std::string tag);
    bool HasTag(std::string tag) const;
    Data GetTagValue(std::string tag) const;
    const std::unordered_map<std::string, Data>& GetTags() const { return tags; }
    
    virtual void Update(float delta_time) {}

    bool IsAttachedTo(WObject* object) const { return IsAttachedToOwner(object) || IsAttachedToParent(object); };
    bool IsAttachedToOwner(WObject* potential_owner) const;
    bool IsAttachedToParent(WObject* potential_parent) const;
    
    std::string name;
    Ref<World> world = nullptr;
    
protected:
    MMatrix4f local_transform = MMatrix4f::identity();
    std::vector<Ref<WObject>> children;
    std::vector<Ref<WObject>> components;
    std::unordered_map<std::string, Data> tags;
    WObject* owner = nullptr;
    WObject* parent = nullptr;
    
    virtual void AttachTo(WObject* new_owner);
    virtual void SetParent(WObject* new_parent);
    void Detach();

};
class WComponent : public WObject {
public:
    WComponent() = delete;
    WComponent(WObject* owner, const std::string& name = "Component");
    bool IsComponent() const override { return true; }
    
    void AddChild(Ref<WObject> child) override {
        throw std::logic_error("Components cannot have children");
    }
    
protected:
    void InitializeAsComponent(WObject* owner);
};

class WActor : public WObject {
public:
    WActor(const std::string& name = "Actor");
    virtual void AttachTo(WObject* new_owner) override { throw std::logic_error("WActor cant be component"); }
};

#endif // WOBJECT_H_A15462527E7955E7_GUARD