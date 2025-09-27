#pragma once
#include "../core/data.h"

#include "../core/wmath.h"
#include "Map.h"
#include "../render/rhi_helpers/IRHIHelper.h"
#include <string>

struct Map;

struct Material
{
    // Material Name
    std::string name = "";

    // Diffuse Map
    Ref<Map> DiffuseMap = nullptr;

    // Specular Glossiness Roughness Map
    Ref<Map> SGRMap = nullptr;

    // Alpha Map
    Ref<Map> AlphaMap = nullptr;
};
DECLARE_DATA_TYPE(Material);
