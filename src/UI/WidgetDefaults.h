#pragma once

#include "../graphic/Image.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace WidgetDefaults
{
    Ref<Image> SolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
}

namespace WidgetVecp
{
    inline unsigned int PackRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
    {
        return (unsigned int(a) << 24) |
               (unsigned int(b) << 16) |
               (unsigned int(g) << 8) |
               unsigned int(r);
    }

    inline unsigned int PackRGBA(const MVector3f& c, float a = 1.0f)
    {
        auto cl = [](float v) {
            int i = (int)(v * 255.0f + 0.5f);
            if (i < 0) i = 0;
            if (i > 255) i = 255;
            return (uint8_t)i;
        };

        return PackRGBA(cl(c[0]), cl(c[1]), cl(c[2]), cl(a));
    }

    Ref<Image> MakeRRect(
        int W,
        int H,
        unsigned int fillColor,
        unsigned int borderColor,
        int cornerRadius,
        int borderThickness,
        unsigned char corners = 0b1111,
        int sub = 3);

    inline size_t Mix(size_t h, size_t v)
    {
        return h ^ (v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2));
    }
}