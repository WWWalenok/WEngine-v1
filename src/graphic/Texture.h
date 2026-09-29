#pragma once
#include "../core/core.h"
#include "../render/IRHIHelper.h"

class Image;

class Texture : public CoreObject  {
public:
    Texture();
    
    void Update();
    
    Property<Ref<Image>> image;
    ReadOnlyProperty<Ref<IRHITexture>> rhi;
    ReadOnlyProperty<bool> dirty;

private:
    Ref<IRHITexture> _rhi = nullptr;
    Ref<Image> _image = nullptr;
    bool _dirty = false;
};