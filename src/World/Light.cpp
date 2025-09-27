#include "Light.h"

Light::Light(std::string name, Type type) 
    : WObject(std::move(name)), light_type(type) {}