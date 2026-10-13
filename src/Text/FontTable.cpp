#include "FontTable.h"

FontTable& FontTable::Get() { static FontTable i; return i; }

Ref<VecpFont> FontTable::LoadVecpFont(const std::string& name,
                                      const void* data, size_t size)
{
    auto it = _sources.find(name);
    if (it != _sources.end()) return it->second;

    auto vf = MakeRef<VecpFont>();
    vf->Load(data, size);
    _sources[name] = vf;
    return vf;
}

Ref<Font> FontTable::GetFont(std::string name, float size)
{
    Key k{name, size};
    auto it = _fonts.find(k);
    if (it != _fonts.end()) return it->second;

    auto sit = _sources.find(name);
    if (sit == _sources.end())
    {
        auto d = GetFont(_default, size);
        return d;
    }

    auto f = Font::Make(name, sit->second, size);
    _fonts[k] = f;
    return f;
}

void FontTable::Clear()
{
    _fonts.clear();
    _sources.clear();
}