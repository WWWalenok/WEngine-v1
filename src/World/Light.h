#pragma once
#include "WObject.h"

class Light : public WObject {
public:
    enum class Type { Directional, Point, Spot };
    
    Light(std::string name = "Light", Type type = Type::Point);
    
    Type light_type = Type::Point;
    MVector3f color = {1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
    float range = 10.0f;
    float spot_angle = 45.0f;
};

DECLARE_DATA_TYPE(Light);