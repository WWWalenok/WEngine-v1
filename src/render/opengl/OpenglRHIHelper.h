#ifndef __OPENGLRHIHELPER_H__
#define __OPENGLRHIHELPER_H__

#include "../../core/core.h"
#include "OpenglRHIResources.h"
#include "OpenglRenderGraphBuilder.h"
#include "OpenglShaderLib.h"

class OpenglRHIHelper : public IRHIHelper
{
public:
    bool inited = false;

    bool Init() override
    {
        auto shaders = ShaderLib::GetShaders();
        for (auto shader : shaders)
        {
            shader.second->Compile();
        }
        inited = true;
        return true;
    }
    bool Inited() override
    {
        return inited;
    }

    OpenglRHIHelper() = default;

    Ref<IRHIMesh> GenMesh() override
    {
        return MakeRef<OpenglRHIMesh>();
    }
    Ref<IRHISkeleton> GenSkeleton() override
    {
        return MakeRef<OpenglRHISkeleton>();
    }
    Ref<IRHITexture> GenTexture() override
    {
        return MakeRef<OpenglRHITexture>();
    }
    Ref<IRHIWindowTarget> GenWindowTarget() override
    {
        return MakeRef<OpenglRHIWindowTarget>();
    }
    Ref<IRHIRenderGraphBuilder> GenRenderGraphBuilder() override
    {
        return MakeRef<OpenglRenderGraphBuilder>();
    }
};

IRHIHelper *GetIRHIHelper()
{
    static OpenglRHIHelper instance;
    return &instance;
}

#endif // __OPENGLRHIHELPER_H__