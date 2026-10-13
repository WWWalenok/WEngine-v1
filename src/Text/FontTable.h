#pragma once
#include "Font.h"
#include <unordered_map>
#include <string>

class FontTable
{
public:
    static FontTable& Get();

    Ref<VecpFont> LoadVecpFont(const std::string& name, const void* data, size_t size);

    Ref<Font> GetFont(std::string name, float size);

    void SetDefault(const std::string& in) {
        _default = in;
    }
    std::string GetDefault() const
    {
        return _default;
    }

    void Clear();

private:
    std::unordered_map<std::string, Ref<VecpFont>> _sources;

    struct Key {
        std::string Name;
        float Size;
        bool operator==(const Key& o) const noexcept {
            return Size == o.Size && Name == o.Name;
        }
    };
    struct KeyHash {
        size_t operator()(const Key& k) const noexcept {
            return std::hash<std::string>()(k.Name)
                 ^ (std::hash<float>()(k.Size) * 0x9e3779b97f4a7c15ULL);
        }
    };
    std::unordered_map<Key, Ref<Font>, KeyHash> _fonts;
    std::string _default;
};