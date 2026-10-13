#include "WidgetDefaults.h"

#include <unordered_map>
#include <algorithm>

namespace WidgetDefaults
{
    Ref<Image> SolidColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        uint32_t key =
            (uint32_t(r) << 24) |
            (uint32_t(g) << 16) |
            (uint32_t(b) << 8) |
            uint32_t(a);

        static std::unordered_map<uint32_t, Ref<Image>> cache;

        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;

        uint8_t px[4] = {r, g, b, a};

        auto img = MakeRef<Image>();
        img->LoadFromMemory(px, sizeof(px), 1, 1, PixelFormat::RGBA_8);

        cache[key] = img;
        return img;
    }
}

namespace WidgetVecp
{
    Ref<Image> MakeRRect(
        int W,
        int H,
        unsigned int fillColor,
        unsigned int borderColor,
        int cornerRadius,
        int borderThickness,
        unsigned char corners,
        int sub)
    {
        if (W <= 0 || H <= 0)
            return nullptr;

        std::vector<uint8_t> buf(size_t(W) * size_t(H) * 4u, 0);
        unsigned int* px = reinterpret_cast<unsigned int*>(buf.data());

        if (borderThickness > 0 && borderColor != 0)
        {
            const int b = borderThickness;
            const int iW = W - 2 * b;
            const int iH = H - 2 * b;
            const int ir = std::max(0, cornerRadius - b);

            vecp::PaintRRect(
                px,
                borderColor,
                0,
                0,
                W,
                H,
                cornerRadius,
                cornerRadius,
                0,
                W,
                corners,
                sub);

            vecp::FillRRect(
                px,
                fillColor,
                b,
                b,
                iW,
                iH,
                ir,
                W,
                corners,
                sub);
        }
        else
        {
            vecp::FillRRect(
                px,
                fillColor,
                0,
                0,
                W,
                H,
                cornerRadius,
                W,
                corners,
                sub);
        }

        auto img = MakeRef<Image>();
        img->LoadFromMemory(
            buf.data(),
            buf.size(),
            (uint32_t)W,
            (uint32_t)H,
            PixelFormat::RGBA_8);

        return img;
    }
}