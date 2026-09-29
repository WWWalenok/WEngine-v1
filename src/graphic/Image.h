#pragma once
#include "../core/core.h"
#include "../render/IRHIHelper.h"

class Image : public CoreObject  {
public:
    Image();
    
    bool LoadFromMemory(const uint8_t* data, size_t data_size, uint32_t width, uint32_t height, PixelFormat format);
    bool LoadFromFile(const std::string& filename);
    
    void Update() { _dirty = false; }
    
    static size_t CalculateDataSize(uint32_t width, uint32_t height, PixelFormat format);

    ReadOnlyProperty<uint32_t> width;
    ReadOnlyProperty<uint32_t> height;
    ReadOnlyProperty<PixelFormat> format;
    ReadOnlyProperty<const uint8_t*> data;
    ReadOnlyProperty<size_t> dataSize;
    ReadOnlyProperty<bool> dirty;

private:
    std::vector<uint8_t> _data;
    uint32_t _width = 0;
    uint32_t _height = 0;
    PixelFormat _format = UNDEFINED;
    bool _dirty = false;
};