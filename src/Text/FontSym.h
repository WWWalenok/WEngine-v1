#pragma once
#include "../core/core.h"

class FontAtlas;

struct FontSym
{
    uint32_t        Codepoint = 0;
    VecpGlyf* Source    = nullptr;

    // Strong-ref: держит атлас живым, пока жив глиф.
    Ref<FontAtlas>  Atlas;

    float Advance  = 0.f;
    float BearingX = 0.f;
    float BearingY = 0.f;
    float Width    = 0.f;
    float Height   = 0.f;

    int   PixelW = 0, PixelH = 0;
    float U0 = 0.f, V0 = 0.f, U1 = 0.f, V1 = 0.f;
    bool  bInAtlas = false;
};