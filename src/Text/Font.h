#pragma once
#include "../core/core.h"
#include "FontSym.h"
#include "../graphic/Image.h"
#include <unordered_map>

class FontAtlas : public CoreObject
{
public:
    static constexpr int Padding = 1;   // 1px по правому/нижнему краю (антиблид)

    struct Region
    {
        int x = 0, y = 0, w = 0, h = 0;
        bool valid() const { return w > 0 && h > 0; }
    };

    Ref<Image> Texture;
    int Width  = 0;
    int Height = 0;

    explicit FontAtlas(int w = 2048, int h = 2048);

    // Пытается выделить регион w×h. Если не влезает — {0,0,0,0}.
    Region Allocate(int w, int h);

    // Пишет пиксели в регион. Помечает Texture dirty.
    void Blit(const Region& r, const uint8_t* rgba, int srcStride = 0);

    // Для отладки/статистики.
    int GetUsedHeight() const { return _totalHeight; }
    size_t GetShelfCount() const { return _shelves.size(); }

private:
    struct Shelf
    {
        int y        = 0;
        int height   = 0;   // уже с Padding
        int usedX    = 0;
    };
    std::vector<Shelf> _shelves;
    int _totalHeight = 0;
};

class Font : public CoreObject
{
public:
    // Стартовый размер атласа. При переполнении растём до MaxAtlasSize.
    static constexpr int StartAtlasSizePow = 8;
    static constexpr int MaxAtlasSizePow   = 13;

    Ref<VecpFont> Source;
    std::string   Name;
    float         Size = 16.f;

    float UnitsPerEm = 1000.f;
    float Ascent = 0.8f, Descent = 0.2f, LineGap = 0.f;

    // Атласы. Заполняются лениво, в порядке наполнения.
    std::vector<Ref<FontAtlas>> Atlases;

    static Ref<Font> Make(const std::string& name, Ref<VecpFont> src, float size);

    float AscentPx()     const { return Ascent  * Size; }
    float DescentPx()    const { return Descent * Size; }
    float LineHeightPx() const { return (Ascent + Descent + LineGap) * Size; }

    Ref<FontSym> GetSym(uint32_t cp);

    bool EnsurePacked(FontSym& sym);

private:
    std::unordered_map<uint32_t, Ref<FontSym>> _glyphs;

    // Пробует упаковать в один атлас. При неудаче — создаёт новый
    // (сначала StartAtlasSize, потом MaxAtlasSize).
    bool _PackInto(FontSym& sym, FontAtlas& atlas, int pw, int ph);
};