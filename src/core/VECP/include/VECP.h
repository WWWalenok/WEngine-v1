#pragma once
#include <math.h>
#include <float.h>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vecp
{
enum e_elememt_type : unsigned char
{
    MOVE_A = 0,
    MOVE_R = 1,
    CLOSE = 2,
    CLOSE_MOVE_A = 3,
    CLOSE_MOVE_R = 4,
    LINE_A = 5,
    LINE_R = 6,
    LINE_H_A = 7,
    LINE_H_R = 8,
    LINE_W_A = 9,
    LINE_W_R = 10,
    BEZIE2_A = 11,
    BEZIE2_R = 12,
    BEZIE2_S_A = 13,
    BEZIE2_S_R = 14,
    BEZIE3_A = 15,
    BEZIE3_R = 16,
    BEZIE3_S_A = 17,
    BEZIE3_S_R = 18,
    BEZIE4_A = 19,
    BEZIE4_R = 20,
    BEZIE4_S_A = 21,
    BEZIE4_S_R = 22,
    ARCPURE = 23,
    ARCSVG_A = 24,
    ARCSVG_R = 25,
    ARC2_A = 26,
    ARC2_R = 27,
    ARC2_S_A = 28,
    ARC2_S_R = 29,
    ARC3_A = 30,
    ARC3_R = 31,
    ARC3_S_A = 32,
    ARC3_S_R = 33,
};

enum e_target_out_type : uint8_t
{
    RGBA_8,
    RGB_8,
    RG_8,
    RED_8,

    RGBA_16,
    RGB_16,
    RG_16,
    RED_16,

    RGBA_F,
    RGB_F,
    RG_F,
    RED_F,

    RGBA_D,
    RGB_D,
    RG_D,
    RED_D,

    // RGBA_4,
    // RGB_4,
    // RG_4,
    // RED_4,

    UNDEFINED // Count of types
};

#pragma pack(push, 1)

struct counter_t_data
{
    uint32_t size = 0;

    uint32_t corner_type = 0;
    double border_inner = 0;
    double border_outer = 0;

    double transparent = 1;
    uint32_t background_color = 0x000000;
    double border_transparent = 1;
    uint32_t border_color     = 0x000000;

    double t_xx = 1, t_xy = 0, t_x = 0, t_yx = 0, t_yy = 1, t_y = 0;

    uint32_t metadata_size = 0;
    uint32_t properties_size = 0;
    uint32_t elements_size = 0;
    uint32_t data_size = 0;
};

struct counter_t : public counter_t_data
{
    // in code support

    uint8_t* metadata = nullptr;
    uint8_t* properties = nullptr;
    e_elememt_type* elements = nullptr;
    double* data = nullptr;

    uint32_t metadata_allocated = 0;
    uint32_t properties_allocated = 0;
    uint32_t elements_allocated = 0;
    uint32_t data_allocated = 0;

    ~counter_t()
    {
        if (metadata)
            delete[] metadata;
        if (properties)
            delete[] properties;
        if (elements)
            delete[] elements;
        if (data)
            delete[] data;
    }
};

struct vector_t_data
{
    uint32_t f_type = 0x56454350;
    uint32_t format = 0;
    uint32_t ts = 0;

    uint32_t size = 0;
    uint32_t type = 0;

    uint32_t width = 0;
    uint32_t heigt = 0;

    double t_xx = 1, t_xy = 0, t_x = 0, t_yx = 0, t_yy = 1, t_y = 0;

    uint32_t metadata_size = 0;
    uint32_t properties_size = 0;
    uint32_t counters_size = 0;
};

struct vector_t : public vector_t_data
{
    // in code support

    uint8_t* metadata = nullptr;
    uint8_t* properties = nullptr;
    counter_t* counters = nullptr;

    uint32_t metadata_allocated = 0;
    uint32_t properties_allocated = 0;
    uint32_t counters_allocated = 0;

    ~vector_t()
    {
        if (metadata)
            delete[] metadata;
        if (properties)
            delete[] properties;
        if (counters)
            delete[] counters;
    }
};

// Один глиф. Соответствует одному codepoint'у.
struct glyf_t_data
{
    uint32_t Codepoint;
    int32_t  advance_width;
    int32_t  lsb;
    uint32_t flags;
    uint32_t glyf_size;
};

struct font_t_data
{
    uint32_t f_type = 0x57464e30;
    uint32_t format = 1;
    uint32_t ts     = 0;
    uint32_t size   = 0;

    int32_t  units_per_em = 0;
    int32_t  ascent       = 0;
    int32_t  descent      = 0;
    int32_t  line_gap     = 0;

    int32_t  x_min = 0, y_min = 0, x_max = 0, y_max = 0;

    uint32_t metadata_size   = 0;
    uint32_t properties_size = 0;
    uint32_t glyfs_size      = 0;
};

struct glyf_t
{
    uint32_t Codepoint   = 0;
    int32_t  advance_width = 0;
    int32_t  lsb         = 0;
    uint32_t flags       = 0;

    vector_t glyf;
};

struct font_t : public font_t_data
{
    uint8_t* metadata   = nullptr;
    uint8_t* properties = nullptr;
    glyf_t*  glyfs      = nullptr;

    uint32_t metadata_allocated   = 0;
    uint32_t properties_allocated = 0;
    uint32_t glyfs_allocated      = 0;

    ~font_t()
    {
        if (metadata)   delete[] metadata;
        if (properties) delete[] properties;
        if (glyfs)      delete[] glyfs;
    }
};

#pragma pack(pop)

void LoadBinary(vector_t* target, const void* data, const size_t size);
bool LoadFontBinary(font_t& font, const void* data, size_t size);
size_t ToBinary(const vector_t* _target, void* _data, size_t buffer_size);
void Render(void* target, uint32_t out_height, uint32_t out_width, e_target_out_type target_type, const vector_t& vector, uint8_t samples = 3, int quality = 0, uint64_t line_size = 0, int alpha_id = 3);

// v is bitmask = 0b1000 for lt | 0b0100 for rt | 0b0010 for rb | 0b0001 for lb 
void FillRRect(unsigned int* data, unsigned int color, int x, int y, int w, int h, int r, int line_size, unsigned char v, int sub = 3);
void PaintRRect(unsigned int* data, unsigned int color, int _x, int _y, int w, int h, int r, int in, int out, int line_size, unsigned char v, int sub = 3);
} // namespace vecp

struct VecpImage
{
    VecpImage();
    VecpImage(const vecp::vector_t& vec);
    VecpImage(const VecpImage&);
    VecpImage(VecpImage&&) noexcept;
    VecpImage& operator=(const VecpImage&);
    VecpImage& operator=(VecpImage&&) noexcept;
    ~VecpImage();

    uint32_t GetCounterCount() const;

    void Set_background_color(uint32_t counter, double r, double g, double b, double a = 1);
    void Set_background_color(uint32_t counter, double* colors);

    void Set_background_color(uint32_t counter, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);
    void Set_background_color(uint32_t counter, uint8_t* colors);

    void Set_border_color(uint32_t counter, double r, double g, double b, double a = 1);
    void Set_border_color(uint32_t counter, double* colors);

    void Set_border_color(uint32_t counter, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xff);
    void Set_border_color(uint32_t counter, uint8_t* colors);

    void Set_transp(uint32_t counter, double transp);
    void Set_border_transp(uint32_t counter, double transp);

    void Set_inner_border(uint32_t counter, double size);
    void Set_outer_border(uint32_t counter, double size);

    void Render(void* target, uint32_t out_height, uint32_t out_width, vecp::e_target_out_type target_type, uint8_t samples = 3, int quality = 0, uint64_t line_size = 0, int alpha_id = 3);


    void Clear_elements();
    void Clear_elements(uint32_t counter);

    void Set_offset(uint32_t counter, double x, double y);

    void Set_offset(double x, double y);

    void Set_scale(uint32_t counter, double x, double y);

    void Set_scale(double x, double y);

    void Set_rot(uint32_t counter, double a);

    void Set_rot(double a);

    void Load(const void* data, const size_t size);
    size_t To(void* data, size_t size);

    void Add_element(uint32_t counter, vecp::e_elememt_type type, const double* data);
    void Add_element(uint32_t counter, vecp::e_elememt_type type, std::initializer_list<double> data)
    {
        Add_element(counter, type, data.begin());
    }

    void Add_elements(uint32_t counter, const vecp::e_elememt_type* types, uint32_t count, const double* data);
    void Add_elements(uint32_t counter, const vecp::e_elememt_type* types, uint32_t count, std::initializer_list<double> data)
    {
        Add_elements(counter, types, count, data.begin());
    }
    void Add_elements(uint32_t counter, std::initializer_list<vecp::e_elememt_type> types, const double* data)
    {
        Add_elements(counter, types.begin(), types.size(), data);
    }
    void Add_elements(uint32_t counter, std::initializer_list<vecp::e_elememt_type> types, std::initializer_list<double> data)
    {
        Add_elements(counter, types.begin(), types.size(), data.begin());
    }
    struct VecpImageImp;
    VecpImageImp* imp;

private:
    void realloc_counters(uint32_t count);
    void realloc_counter_elements(uint32_t counter, uint32_t count);
    void realloc_counter_data(uint32_t counter, uint32_t count);

    vecp::counter_t* getc(uint32_t counter);
};

struct VecpGlyf
{
    uint32_t Codepoint;
    int32_t  advance_width;
    int32_t  lsb;
    uint32_t flags;
    double bx0, by0, bx1, by1;
    VecpImage glyf;
};

struct VecpFont
{
    VecpGlyf* GetGlyph(const uint32_t& codepoint) const;
    void Load(const void* data, const size_t size);
    size_t To(void* data, size_t size);

    vecp::font_t font;

private:
    std::unordered_map<uint32_t, VecpGlyf> codepoint_map;
};