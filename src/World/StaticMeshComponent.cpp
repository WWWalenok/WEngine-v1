#include "StaticMeshComponent.h"
#include "../graphic/Mesh.h"
#include "../graphic/Material.h"

StaticMeshComponent::StaticMeshComponent(Ref<WObject> owner, const std::string& name)
    : WComponent(owner, name)
    , mesh(
        [this]() { return _mesh; },
        [this](Ref<Mesh> value) { _mesh = value; }
    )
    , material(
        [this]() { return _material; },
        [this](Ref<Material> value) { _material = value; }
    )
    , isVisible(
        [this]() { return _isVisible; },
        [this](bool value) { _isVisible = value; }
    )
    , castShadow(
        [this]() { return _castShadow; },
        [this](bool value) { _castShadow = value; }
    )
    , receiveShadow(
        [this]() { return _receiveShadow; },
        [this](bool value) { _receiveShadow = value; }
    )
    , dirty(
        [this]() { return _dirty; }
    )
{
    mesh = nullptr;
    material = nullptr;
    isVisible = true;
    castShadow = true;
    receiveShadow = true;
}

void StaticMeshComponent::OnMeshChanged() {
    _dirty = true;
    UpdateBoundingBox();
}

void StaticMeshComponent::OnMaterialsChanged() {
    _dirty = true;
}

void StaticMeshComponent::PrepareForRendering() {

}

void StaticMeshComponent::OnRender() {
    _dirty = false;
}

void StaticMeshComponent::UpdateBoundingBox() {
    if (_mesh) {
        auto world_transform = GetWorldTransform();
        
    } else {
        
    }
}