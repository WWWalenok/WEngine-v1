#include "Texture.h"
#include "Image.h"

Texture::Texture():
    image(
        [this]() { return _image; },
        [this](Ref<Image> value) { _image = value; _dirty = true; }
    ),
    rhi(
        [this]() { return _rhi; }
    ),
    dirty(
        [this]() { return _dirty; }
    )
{
}

void Texture::Update() {
    if (!_rhi) {
        _rhi = IRHIHelper::Get()->GenTexture();
    }
    
    if (_dirty && _image && _rhi) {
        _rhi->Update(this);
        _dirty = false;
    }
}