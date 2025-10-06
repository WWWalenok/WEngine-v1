#pragma once
#include "../core/core.h"
#include "../render/rhi_helpers/IRHIHelper.h"

class Texture;
class RenderTarget;
class ProceduralTexture;


class Map {
public:
    Map(MVector3f color) : color(color) {}
    Map(Ref<Texture> texture) : texture(texture) {}
    Map(Ref<RenderTarget> renderTarget) : renderTarget(renderTarget) {}
    Map(Ref<ProceduralTexture> proceduralTex) : proceduralTex(proceduralTex) {}
    
    Ref<Texture> GetTexture() const { return texture; }
    Ref<RenderTarget> GetRenderTarget() const { return renderTarget; }
    Ref<ProceduralTexture> GetProcedural() const { return proceduralTex; }
    MVector3f GetColor() const { return color; }

private:
    MVector3f color = MVector3f(0.0f,0.0f,0.0f);
    Ref<Texture> texture = nullptr;
    Ref<RenderTarget> renderTarget = nullptr;
    Ref<ProceduralTexture> proceduralTex = nullptr;
};
DECLARE_DATA_TYPE(Map);
