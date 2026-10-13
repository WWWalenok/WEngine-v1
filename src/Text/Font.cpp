#include "Font.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <cstdint>


FontAtlas::FontAtlas(int w, int h)
{
    Width  = std::max(1, w);
    Height = std::max(1, h);

    Texture = MakeRef<Image>();
    std::vector<uint8_t> zero(size_t(Width) * size_t(Height) * 4u, 0);
    Texture->LoadFromMemory(zero.data(), zero.size(),
                            (uint32_t)Width, (uint32_t)Height,
                            PixelFormat::RED_8);
    Texture->Update();
}

FontAtlas::Region FontAtlas::Allocate(int w, int h)
{
    if (w <= 0 || h <= 0) return {};
    if (w > Width || h > Height) return {};  // принципиально не влезает

    const int needW = w + Padding;
    const int needH = h + Padding;

    // Best-fit по высоте: берём самую узкую полку, которая влезает
    // по высоте и по остатку ширины. Так короткие глифы не тратят
    // высокие полки, оставшиеся для высоких.
    int bestIdx    = -1;
    int bestHeight = INT32_MAX;

    for (size_t i = 0; i < _shelves.size(); ++i)
    {
        const auto& s = _shelves[i];
        if (s.height < needH)              continue;
        if (s.usedX + needW > Width)       continue;
        if (s.height >= bestHeight)        continue;

        bestHeight = s.height;
        bestIdx    = (int)i;
    }

    if (bestIdx >= 0)
    {
        auto& s = _shelves[bestIdx];
        Region r{ s.usedX, s.y, w, h };
        s.usedX += needW;
        return r;
    }

    // Новая полка.
    if (_totalHeight + needH > Height) return {};

    Shelf s{ _totalHeight, needH, needW };
    _shelves.push_back(s);
    _totalHeight += needH;

    return Region{ 0, s.y, w, h };
}

void FontAtlas::Blit(const Region& r, const uint8_t* data, int srcStride)
{
    if (!r.valid() || !data || !Texture) return;
    if (srcStride == 0) srcStride = r.w;

    const int rowBytes = r.w;

    if (srcStride == rowBytes)
    {
        Texture->WriteRegion(r.x, r.y, r.w, r.h, data);
    }
    else
    {
        // Собираем плотный буфер построчно.
        std::vector<uint8_t> tight(size_t(rowBytes) * r.h);
        for (int row = 0; row < r.h; ++row)
            std::memcpy(tight.data() + size_t(row) * rowBytes,
                        data + size_t(row) * srcStride,
                        rowBytes);
        Texture->WriteRegion(r.x, r.y, r.w, r.h, tight.data());
    }
}

Ref<Font> Font::Make(const std::string& name, Ref<VecpFont> src, float size)
{
    auto f = MakeRef<Font>();
    f->Source = std::move(src);
    f->Size = size;

    if (f->Source)
    {
        const auto& fd = f->Source->font;
        f->UnitsPerEm = fd.units_per_em > 0 ? (float)fd.units_per_em : 1000.f;
        f->Ascent  =  (float)fd.ascent   / f->UnitsPerEm;
        f->Descent = -(float)fd.descent  / f->UnitsPerEm;  // descent<0 обычно
        f->LineGap =  (float)fd.line_gap / f->UnitsPerEm;
        f->Name    = std::move(name);
    }
    return f;
}

Ref<FontSym> Font::GetSym(uint32_t cp)
{
    auto it = _glyphs.find(cp);
    if (it != _glyphs.end()) return it->second;

    auto sym = MakeRef<FontSym>();
    sym->Codepoint = cp;

    if (Source)
    {
        VecpGlyf* g = Source->GetGlyph(cp);
        if (g)
        {
            sym->Source   = g;
            sym->Advance  = (float)g->advance_width / UnitsPerEm;
            sym->BearingX = (float)g->lsb           / UnitsPerEm;
            sym->BearingY = (float)-g->by0           / UnitsPerEm;
            sym->Width    = (g->bx1 - g->bx0) / UnitsPerEm;
            sym->Height   = (g->by1 - g->by0) / UnitsPerEm;
        }
    }

    _glyphs[cp] = sym;
    EnsurePacked(*sym);
    return sym;
}

bool Font::_PackInto(FontSym& sym, FontAtlas& atlas, int pw, int ph)
{
    auto r = atlas.Allocate(pw, ph);
    if (!r.valid()) return false;

    sym.Atlas  = &atlas;
    sym.PixelW = pw;
    sym.PixelH = ph;
    sym.U0 = float(r.x)      / atlas.Width;
    sym.V0 = float(r.y)      / atlas.Height;
    sym.U1 = float(r.x + pw) / atlas.Width;
    sym.V1 = float(r.y + ph) / atlas.Height;
    sym.bInAtlas = true;

    std::vector<uint8_t> raster(size_t(pw) * size_t(ph), 0);
    sym.Source->glyf.Set_scale(Size / UnitsPerEm, Size / UnitsPerEm);
    sym.Source->glyf.Set_offset(-Size / UnitsPerEm * sym.Source->bx0, -Size / UnitsPerEm * sym.Source->by0);
    for(int c = 0; c < sym.Source->glyf.GetCounterCount(); ++c)
    {
        sym.Source->glyf.Set_background_color(c, 1.f, 1.f, 1.f, 1.f);
        sym.Source->glyf.Set_border_color(c, 1.f, 1.f, 1.f, 1.f);
    }
    sym.Source->glyf.Render(raster.data(), (uint32_t)ph, (uint32_t)pw, vecp::RED_8, 3, 0, 0, 0);

    atlas.Blit(r, raster.data());
    return true;
}

bool Font::EnsurePacked(FontSym& sym)
{
    static constexpr int StartAtlasSize = 1 << StartAtlasSizePow;
    static constexpr int MaxAtlasSize   = 1 << MaxAtlasSizePow;
    if (sym.bInAtlas) return true;

    const int pw = (int)std::ceil(sym.Width  * Size);
    const int ph = (int)std::ceil(sym.Height * Size);

    if (pw <= 0 || ph <= 0 || !sym.Source)
    {
        sym.bInAtlas = true;
        sym.PixelW = sym.PixelH = 0;
        sym.Atlas = nullptr;
        return true;
    }

    if (pw > MaxAtlasSize || ph > MaxAtlasSize)
    {
        sym.bInAtlas = true;
        sym.PixelW = sym.PixelH = 0;
        sym.Atlas = nullptr;
        return false;
    }

    for (auto& a : Atlases)
    {
        if (_PackInto(sym, *a, pw, ph)) return true;
    }

    int sz = Atlases.size() && Atlases.back() ? std::min(Atlases.back()->Height * 2, MaxAtlasSize) : StartAtlasSize;
    while (pw > sz || ph > sz) sz = std::min(sz * 2, MaxAtlasSize);

    auto a = MakeRef<FontAtlas>(sz, sz);
    if (_PackInto(sym, *a, pw, ph))
    {
        Atlases.push_back(a);
        return true;
    }
    
    sym.bInAtlas = true;
    sym.PixelW = sym.PixelH = 0;
    sym.Atlas = nullptr;
    return false;
}
