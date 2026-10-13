#include "Image.h"
#include <cstring>

size_t Image::BytesPerPixel(PixelFormat fmt)
{
    switch (fmt) {
        case RGBA_8: case RGB_8: case RG_8: case RED_8: return 1 * ((fmt==RGBA_8)?4:(fmt==RGB_8)?3:(fmt==RG_8)?2:1);
        default: return 0;
    }
}

Image::Image():
    width(
        [this]() { return _width; }
    ),
    height(
        [this]() { return _height; }
    ),
    format(
        [this]() { return _format; }
    ),
    data(
        [this]() { return _data.data(); }
    ),
    dataSize(
        [this]() { return _data.size(); }
    ),
    dirty(
        [this]() { return _dirty; }
    )
{
}

bool Image::LoadFromMemory(const uint8_t* data, size_t data_size, uint32_t width, uint32_t height, PixelFormat format) {
    if (!data || data_size == 0) return false;
    
    size_t expected_size = CalculateDataSize(width, height, format);
    if (data_size < expected_size) return false;
    
    _width = width;
    _height = height;
    _format = format;
    
    _data.resize(expected_size);
    memcpy(_data.data(), data, expected_size);
    _dirty = true;
    return true;
}

bool Image::LoadFromFile(const std::string& filename) {
    // TODO
    return false;
}

void Image::WriteRegion(int x, int y, int w, int h, const void* data)
{
    if (!data || w <= 0 || h <= 0) return;
    if (x < 0 || y < 0) return;
    if (x + w > (int)_width || y + h > (int)_height) return;

    size_t bpp = 0;
    switch (_format) {
        case RGBA_8: bpp = 4; break;
        case RGB_8:  bpp = 3; break;
        case RG_8:   bpp = 2; break;
        case RED_8:  bpp = 1; break;
        default: return;
    }

    const size_t rowBytes = size_t(w) * bpp;
    const size_t dstPitch = size_t(_width) * bpp;
    uint8_t* dst = _data.data() + size_t(y) * dstPitch + size_t(x) * bpp;
    const uint8_t* src = (const uint8_t*)data;

    for (int row = 0; row < h; ++row) {
        memcpy(dst, src, rowBytes);
        dst += dstPitch;
        src += rowBytes;
    }
    _dirty = true;
}

void Image::AttachVecp(Ref<VecpImage> vecp)
{
    _vecp = vecp;
}

bool Image::RasterizeVecp(uint32_t width, uint32_t height, PixelFormat format)
{
    if (!_vecp) return false;
    if (width == 0 || height == 0) return false;

    if (format != RGBA_8) return false;

    size_t bytes = CalculateDataSize(width, height, format);
    if (bytes == 0) return false;

    _width  = width;
    _height = height;
    _format = format;
    _data.assign(bytes, 0);

    // Порядок аргументов у VecpImage::Render — (target, H, W, type).
    _vecp->Render(_data.data(), height, width, vecp::RGBA_8);

    _dirty = true;
    return true;
}

size_t Image::CalculateDataSize(uint32_t width, uint32_t height, PixelFormat format) {
    size_t component_count = 0;
    size_t component_size = 0;
    
    switch (format) {
        case RGBA_8: case RGB_8: case RG_8: case RED_8:
            component_size = 1;
            break;
        case RGBA_16: case RGB_16: case RG_16: case RED_16:
            component_size = 2;
            break;
        case RGBA_32: case RGB_32: case RG_32: case RED_32:
        case RGBA_F: case RGB_F: case RG_F: case RED_F:
            component_size = 4;
            break;
        default:
            return 0;
    }
    
    switch (format) {
        case RED_8: case RED_16: case RED_32: case RED_F:
            component_count = 1;
            break;
        case RG_8: case RG_16: case RG_32: case RG_F:
            component_count = 2;
            break;
        case RGB_8: case RGB_16: case RGB_32: case RGB_F:
            component_count = 3;
            break;
        case RGBA_8: case RGBA_16: case RGBA_32: case RGBA_F:
            component_count = 4;
            break;
        default:
            return 0;
    }
    
    return width * height * component_count * component_size;
}