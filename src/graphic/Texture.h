#pragma once
#include "../core/data.h"
#include "../render/rhi_helpers/IRHIHelper.h"

class Texture {
public:
    Texture(std::string path)
    {
        rhimesh = IRHIHelper::Get()->GenTexture();
        rhimesh->Update(path);
    }
    Texture(void* data, PixelFormat type, size_t w, size_t h)
    {
        rhimesh = IRHIHelper::Get()->GenTexture();
        rhimesh->Update(data, type, w, h);
    }
    Texture() {}

private:
    Ref<IRHITexture> rhimesh  = nullptr;
};
DECLARE_DATA_TYPE(Texture);
