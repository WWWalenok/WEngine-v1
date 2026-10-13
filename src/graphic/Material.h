#pragma once
#include "../core/core.h"

class Map;

class Material
{
public:
    std::string name = "";
    std::string type = "";
    std::unordered_map<std::string, Ref<Map>> maps;
};
