#ifndef __OPENGLRHIHELPER_H__
#define __OPENGLRHIHELPER_H__

#include "../../core/core.h"
#include "../IRHIHelper.h"

class OpenglRHIHelper : public IRHIHelper
{
public:
    bool inited = false;

    bool Init() override;
    bool Inited() override;

    OpenglRHIHelper() = default;

    Ref<IRHIMesh> GenMesh() override;
    Ref<IRHISkeleton> GenSkeleton() override;
    Ref<IRHITexture> GenTexture() override;
    Ref<IRHIWindowTarget> GenWindowTarget() override;
    Ref<IRHIRenderGraphBuilder> GenRenderGraphBuilder() override;
};

#endif // __OPENGLRHIHELPER_H__