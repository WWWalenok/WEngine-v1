#pragma once
#include "../core/core.h"

struct Map;

struct Material
{
    std::string name = "";
    std::string type = "";
    std::unordered_map<std::string, Ref<Map>> _maps;
};
