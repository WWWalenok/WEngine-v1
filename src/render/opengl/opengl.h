#pragma once
#include "def.h"
#if WITH_H
#include "OpenglRenderGraphBuilder.h" 
#include "OpenglRenderNodes.h" 
#include "OpenglRHIResources.h"
#include "OpenglRHIHelper.h"

bool OpenglRHIHelper::Init()
{
    auto shaders = ShaderLib::GetShaders();
    for (auto shader : shaders)
    {
        shader.second->Compile();
    }
    inited = true;
    return true;
}
bool OpenglRHIHelper::Inited() 
{
    return inited;
}
Ref<IRHIMesh> OpenglRHIHelper::GenMesh() 
{
    return MakeRef<OpenglRHIMesh>();
}
Ref<IRHISkeleton> OpenglRHIHelper::GenSkeleton() 
{
    return MakeRef<OpenglRHISkeleton>();
}
Ref<IRHITexture> OpenglRHIHelper::GenTexture() 
{
    return MakeRef<OpenglRHITexture>();
}
Ref<IRHIWindowTarget> OpenglRHIHelper::GenWindowTarget() 
{
    return MakeRef<OpenglRHIWindowTarget>();
}
Ref<IRHIRenderGraphBuilder> OpenglRHIHelper::GenRenderGraphBuilder() 
{
    return MakeRef<OpenglRenderGraphBuilder>();
}
#endif