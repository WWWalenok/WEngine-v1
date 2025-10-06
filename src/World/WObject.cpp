#include "WObject.h"
#include "World.h"
#include <stdexcept>

WObject::WObject(const std::string& name) : name(name) {
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
    if (owner) {
        throw std::logic_error("Object is already attached");
    }
    
    owner = new_owner;
    world = owner->world;
}

void WObject::SetParent(WObject* new_parent) {
    if (parent) {
        throw std::logic_error("Object is already attached");
    }
    
    parent = new_parent;
    world = new_parent->world;
}

void WObject::Detach() {
    if(owner)
        owner->RemoveComponent(this);
    owner = nullptr;
    if(parent)
        parent->RemoveChild(this);
    parent = nullptr;
}

bool WObject::IsAttachedToOwner(WObject* potential_owner) const {
    if (!potential_owner) return false;
    
    const WObject* current = this;
    while (current) {
        if (current == potential_owner) return true;
        current = current->owner;
    }
    return false;
}

bool WObject::IsAttachedToParent(WObject* potential_parent) const {
    if (!potential_parent) return false;
    
    const WObject* current = this;
    while (current) {
        if (current == potential_parent) return true;
        current = current->parent;
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

void WObject::AddTag(std::string tag, Data value) {
    tags.insert({tag, value});
    if (world) world->RegisterTags(Ref<WObject>(this));
}

void WObject::RemoveTag(std::string tag) {
    tags.erase(tag);
    if (world) world->UnregisterTags(Ref<WObject>(this));
}

bool WObject::HasTag(std::string tag) const {
    return tags.find(tag) != tags.end();
}

Data WObject::GetTagValue(std::string tag) const {
    auto v = tags.find(tag);
    return (v != tags.end()) ? v->second : Data();
}

// WComponent

WComponent::WComponent(WObject* owner, const std::string& name)
    : WObject(std::move(name)) 
{
    if (!owner) {
        throw std::invalid_argument("Component must have an owner");
    }
    InitializeAsComponent(owner);
}

void WComponent::InitializeAsComponent(WObject* owner) {
    owner->AddComponent(this);
}

// WActor

WActor::WActor(const std::string& name) : WObject(name) {}