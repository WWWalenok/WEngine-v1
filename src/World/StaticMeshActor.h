#pragma once
#include "StaticMeshComponent.h"

class StaticMeshActor : public WActor {
public:
    StaticMeshActor(const std::string& name = "StaticMeshActor");

    WeakProperty<Ref<Mesh>> mesh;
    WeakProperty<Ref<Material>> material;
    WeakProperty<bool> isVisible;
    WeakProperty<bool> castShadow;
    WeakProperty<bool> receiveShadow;
    WeakReadOnlyProperty<bool> dirty;

    Ref<StaticMeshComponent> GetOwnedMeshComponent() const { return meshComponent; }

private:
    Ref<StaticMeshComponent> meshComponent;
};