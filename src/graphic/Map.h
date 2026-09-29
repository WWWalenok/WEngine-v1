#pragma once
#include "../core/core.h"
#include "../render/IRHIHelper.h"

class Texture;
class RenderTarget;
class ProceduralTexture;


class Map : public CoreObject  {
public:
    Map();
    Map(MVector3f);
    Map(Ref<Texture>);

    Property<MVector3f> color;
    Property<Ref<Texture>> texture;

private:
    void _reset();
    MVector3f _color;
    Ref<Texture> _texture;
};

