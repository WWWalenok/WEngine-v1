#pragma once
#include "../core/core.h"
#include "../render/IRHIHelper.h"

class Image : public CoreObject  {
public:
    Image();
    
    bool LoadFromMemory(const uint8_t* data, size_t data_size, uint32_t width, uint32_t height, PixelFormat format);
    bool LoadFromFile(const std::string& filename);

    void WriteRegion(int x, int y, int w, int h, const void* data);

    void AttachVecp(Ref<VecpImage> vecp);
    bool HasVecp() const { return _vecp != nullptr; }
    VecpImage* GetVecp() { return _vecp.get(); }
    const VecpImage* GetVecp() const { return _vecp.get(); }
    bool RasterizeVecp(uint32_t width, uint32_t height, PixelFormat format = RGBA_8);
    void Update() { _dirty = false; }
    
    static size_t CalculateDataSize(uint32_t width, uint32_t height, PixelFormat format);

    ReadOnlyProperty<uint32_t> width;
    ReadOnlyProperty<uint32_t> height;
    ReadOnlyProperty<PixelFormat> format;
    ReadOnlyProperty<const uint8_t*> data;
    ReadOnlyProperty<size_t> dataSize;
    ReadOnlyProperty<bool> dirty;

private:
    static size_t BytesPerPixel(PixelFormat fmt);
    std::vector<uint8_t> _data;
    uint32_t _width = 0;
    uint32_t _height = 0;
    PixelFormat _format = UNDEFINED;
    Ref<VecpImage> _vecp;
    bool _dirty = false;
};