#pragma once
#include "WObject.h"
class Mesh;
class Material;

class StaticMeshComponent : public WComponent {
public:
    StaticMeshComponent(Ref<WObject> owner, const std::string& name = "StaticMeshComponent");
    
    Property<Ref<Mesh>> mesh;
    Property<Ref<Material>> material;
    Property<bool> isVisible;
    Property<bool> castShadow;
    Property<bool> receiveShadow;
    ReadOnlyProperty<bool> dirty;

    virtual void OnRender();

protected:
    virtual void OnMeshChanged();
    virtual void OnMaterialsChanged();
    virtual void PrepareForRendering();
    virtual void UpdateBoundingBox();

private:
    Ref<Mesh> _mesh;
    Ref<Material> _material;
    bool _isVisible;
    bool _castShadow;
    bool _receiveShadow;
    bool _dirty;

};
