#pragma once
#include "../core/core.h"
#include "../render/rhi_helpers/IRHIHelper.h"

class Texture {
public:
    Texture() {}

    void Update()
    {
        if(!rhi)
        {
            rhi = IRHIHelper::Get()->GenTexture();
            rhi->Update(this);
        }
    }

private:
    Ref<IRHITexture> rhi  = nullptr;
};
DECLARE_DATA_TYPE(Texture);
