#include "StaticMeshActor.h"

StaticMeshActor::StaticMeshActor(const std::string& name)
    : WActor(name)
{
    Ref<StaticMeshActor> Self = GetSelfRef(this);
    meshComponent = MakeRef<StaticMeshComponent>(Self, name + "_component");
    
    mesh = meshComponent->mesh;
    material = meshComponent->material;
    isVisible = meshComponent->isVisible;
    castShadow = meshComponent->castShadow;
    receiveShadow = meshComponent->receiveShadow;
    dirty = meshComponent->dirty;
}