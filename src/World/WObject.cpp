#include "WObject.h"
#include "World.h"
#include <stdexcept>
#include "../game/GameSystem.h"

WObject::WObject(const std::string& name) : name(name) {
    local_transform = MMatrix4f::identity();
    world_transform = MMatrix4f::identity();
    world_transform_dirty = true;
}

void WObject::SetTransformMode(TransformMode mode) {
    if (transform_mode == mode) return;
    transform_mode = mode;
    if (mode == TransformMode::Strong) {
        UpdateWorldTransformRecursive();
    }
}

void WObject::EnsureWorldTransform() const {
    if (!world_transform_dirty) return;

    MMatrix4f result;
    if (auto l_parent = parent.lock()) {
        result = l_parent->GetWorldTransform() * local_transform;
    } else if (auto l_owner = owner.lock()) {
        result = l_owner->GetWorldTransform() * local_transform;
    } else {
        result = local_transform;
    }
    world_transform.Get() = result;
    world_transform_dirty = false;
}

void WObject::MarkTransformDirty() {
    world_transform_dirty = true;
    for (auto& child : children) child->MarkTransformDirty();
    for (auto& comp : components) comp->MarkTransformDirty();
}

void WObject::UpdateWorldTransformRecursive() {
    MMatrix4f result;
    if (auto l_parent = parent.lock()) {
        result = l_parent->GetWorldTransform() * local_transform;
    } else if (auto l_owner = owner.lock()) {
        result = l_owner->GetWorldTransform() * local_transform;
    } else {
        result = local_transform;
    }
    world_transform.Get() = result;
    world_transform_dirty = false;

    for (auto& child : children) child->UpdateWorldTransformRecursive();
    for (auto& comp : components) comp->UpdateWorldTransformRecursive();
}

void WObject::OnTransformChanged() {
    if (transform_mode == TransformMode::Strong || world_transform.IsExternal()) {
        UpdateWorldTransformRecursive();
    } else {
        MarkTransformDirty();
    }
}

void WObject::BindWorldTransformStorage(MMatrix4f* ptr) {
    if (!ptr) {
        UnbindWorldTransformStorage();
        return;
    }
    world_transform.BindExternal(ptr);
    world_transform_dirty = true;
    EnsureWorldTransform();
}

void WObject::UnbindWorldTransformStorage() {
    world_transform.UnbindExternal();
}

void WObject::UpdateImp(float delta_time)
{
    WL_START_TYMETRACE;
    if(!can_tick)
    {
        return;
    }
    
    Update(delta_time);
    for (auto comp : components)
    {
        comp->UpdateImp(delta_time);
    }
    for (auto child : children)
    {
        child->UpdateImp(delta_time);
    }
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

    child->OnTransformChanged();
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

    component->OnTransformChanged();
}

void WObject::RemoveComponent(Ref<WObject> component) {
    auto it = std::find(components.begin(), components.end(), component);
    if (it != components.end()) {
        (*it)->Detach();
        components.erase(it);
    }
}

void WObject::AttachTo(Ref<WObject> new_owner) {
    if (owner) {
        throw std::logic_error("Object is already attached");
    }

    owner = new_owner;
    world = new_owner->world;
    OnTransformChanged();
}

void WObject::SetParent(Ref<WObject> new_parent) {
    if (parent) {
        throw std::logic_error("Object is already attached");
    }

    parent = new_parent;
    world = new_parent->world;
    OnTransformChanged();
}

void WObject::Detach() {
    if(auto l_owner = owner.lock())
        l_owner->RemoveComponent(this);
    owner = nullptr;
    if(auto l_parent = parent.lock())
        l_parent->RemoveChild(this);
    parent = nullptr;
    OnTransformChanged();
}

bool WObject::IsAttachedToOwner(Ref<WObject> potential_owner) const {
    if (!potential_owner) return false;

    const WObject* current = this;
    while (current) {
        if (current == potential_owner.get()) return true;
        current = current->owner.lock().get();
    }
    return false;
}

bool WObject::IsAttachedToParent(Ref<WObject> potential_parent) const {
    if (!potential_parent) return false;

    const WObject* current = this;
    while (current) {
        if (current == potential_parent.get()) return true;
        current = current->parent.lock().get();
    }
    return false;
}

// ----- Local transform setters -----

void WObject::SetTransform(MMatrix4f transform) {
    local_transform = transform;
    OnTransformChanged();
}

void WObject::SetRotation(MVector3f rotation) {
    wm::SetRotation(local_transform, rotation);
    OnTransformChanged();
}

void WObject::SetRotation(MVector4f quaternion) {
    wm::SetRotation(local_transform, quaternion);
    OnTransformChanged();
}

void WObject::SetScale(MVector3f scale) {
    wm::SetScale(local_transform, scale);
    OnTransformChanged();
}

void WObject::SetPosition(MVector3f position) {
    wm::SetTranslation(local_transform, position);
    OnTransformChanged();
}
// ----- World transform setters -----

void WObject::SetWorldTransform(MMatrix4f worldTransform) {
    if (auto l_parent = parent.lock()) {
        local_transform = l_parent->GetWorldTransform().invert() * worldTransform;
    } else if (auto l_owner = owner.lock()) {
        local_transform = l_owner->GetWorldTransform().invert() * worldTransform;
    } else {
        local_transform = worldTransform;
    }
    OnTransformChanged();
}

void WObject::SetWorldPosition(MVector3f worldPos) {
    MMatrix4f worldMat = GetWorldTransform();
    wm::SetTranslation(worldMat, worldPos);
    SetWorldTransform(worldMat);
}

void WObject::SetWorldRotation(MVector3f euler) {
    MMatrix4f worldMat = GetWorldTransform();
    wm::SetRotation(worldMat, euler);
    SetWorldTransform(worldMat);
}

void WObject::SetWorldRotation(MVector4f quat) {
    MMatrix4f worldMat = GetWorldTransform();
    wm::SetRotation(worldMat, quat);
    SetWorldTransform(worldMat);
}

void WObject::SetWorldScale(MVector3f scale) {
    MMatrix4f worldMat = GetWorldTransform();
    wm::SetScale(worldMat, scale);
    SetWorldTransform(worldMat);
}

// ----- Getters -----

MVector3f WObject::GetPosition() const {
    return wm::GetTranslation(local_transform);
}

MVector4f WObject::GetRotation() const {
    return wm::GetRotation(local_transform);
}

MVector3f WObject::GetScale() const {
    return wm::GetScale(local_transform);
}

MVector3f WObject::GetWorldPosition() const {
    return wm::GetTranslation(GetWorldTransform());
}

MVector4f WObject::GetWorldRotation() const {
    return wm::GetRotation(GetWorldTransform());
}

MVector3f WObject::GetWorldScale() const {
    return wm::GetScale(GetWorldTransform());
}

MMatrix4f WObject::ToLocal(const MMatrix4f& worldMatrix) const {
    MMatrix4f worldToLocal = GetWorldTransform().invert();
    return worldToLocal * worldMatrix;
}

MMatrix4f WObject::ToWorld(const MMatrix4f& localMatrix) const {
    return GetWorldTransform() * localMatrix;
}

MVector3f WObject::ToLocalPoint(const MVector3f& worldPoint) const {
    return MVector3f();
}

MVector3f WObject::ToWorldPoint(const MVector3f& localPoint) const {
    return MVector3f();
}

// ----- Tags -----

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

// ----- WComponent -----

WComponent::WComponent(Ref<WObject> owner, const std::string& name)
    : WObject(std::move(name))
{
    if (!owner) {
        throw std::invalid_argument("Component must have an owner");
    }
    InitializeAsComponent(owner);
}

void WComponent::InitializeAsComponent(Ref<WObject> owner) {
    auto SelfRef = GetSelfRef(this);
    owner->AddComponent(SelfRef);
}

// ----- WActor -----

WActor::WActor(const std::string& name) : WObject(name) {}