#include "Map.h"
#include "Texture.h"

Map::Map():
    color(
        [this]() { return _color; },
        [this](MVector3f value) { _reset(); _color = value; }
    ),
    texture(
        [this]() { return _texture; },
        [this](Ref<Texture> value) { _reset(); _texture = value; }
    )
{

};

Map::Map(MVector3f value) : Map() { _color = value; }
Map::Map(Ref<Texture> value) : Map() { _texture = value; }

void Map::_reset() { 
    _color = {0.0f, 0.0f, 0.0f, 0.0f};
    _texture = nullptr;
}