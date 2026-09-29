#ifndef __IRHIHELPER_H__
#define __IRHIHELPER_H__

#include "../core/core.h"

class IRHIHelper;
IRHIHelper *GetIRHIHelper();

// Предварительные объявления
struct World;
struct Camera;
struct UI;
struct Skeleton;
struct Mesh;
struct SkeletalMesh;
struct ProceduralTexture;
struct Texture;
struct IWindow;
class RenderGraphBuilder;

enum PixelFormat : uint8_t
{
    RGBA_8,
    RGB_8,
    RG_8,
    RED_8,

    RGBA_16,
    RGB_16,
    RG_16,
    RED_16,

    RGBA_32,
    RGB_32,
    RG_32,
    RED_32,

    RGBA_F,
    RGB_F,
    RG_F,
    RED_F,

    UNDEFINED
};

// Superclass for all RHI implementations
class IRHIResource
{
public:
    virtual size_t GetType() const = 0;
    virtual ~IRHIResource() = default;
    static size_t StaticGetType()           
    {                                       
        return 0;            
    }
};

template<typename T, typename check = std::enable_if<std::is_base_of<IRHIResource, T>::value>::type>
static Ref<T> ResourceCast(Ref<IRHIResource> ref)
{
    if (ref->GetType() == T::StaticGetType())
        return StaticRefCast<T>(ref);
    return nullptr;
}

#define RHI_RESOURCE_IMP(CPUResource) virtual void Update(CPUResource *) = 0;

#define RHI_TYPE_IMP(GPUResource)           \
    virtual size_t GetType() const override \
    {                                       \
        return GID(GPUResource);            \
    }                                       \
    static size_t StaticGetType()           \
    {                                       \
        return GID(GPUResource);            \
    }

template<typename T>
class TrivialRHIResource;

#define DEFINE_TRIVIAL_RESOURCE(T)                         \
    template<>                                             \
    class TrivialRHIResource<T> : public IRHIResource      \
    {                                                      \
        T data;                                            \
                                                           \
    public:                                                \
        RHI_TYPE_IMP(T);                                   \
        TrivialRHIResource<T>(const T &val) : data(val){}; \
        const T &Get() const                               \
        {                                                  \
            return data;                                   \
        };                                                 \
        void Set(const T &val)                             \
        {                                                  \
            data = val;                                    \
        };                                                 \
    }


DEFINE_TRIVIAL_RESOURCE(uint32_t);

DEFINE_TRIVIAL_RESOURCE(float);

DEFINE_TRIVIAL_RESOURCE(MVector2f);
DEFINE_TRIVIAL_RESOURCE(MVector3f);
DEFINE_TRIVIAL_RESOURCE(MVector4f);

DEFINE_TRIVIAL_RESOURCE(MMatrix2f);
DEFINE_TRIVIAL_RESOURCE(MMatrix3f);
DEFINE_TRIVIAL_RESOURCE(MMatrix4f);

class IRHISkeleton : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHISkeleton)
    RHI_RESOURCE_IMP(Skeleton)
};

class IRHIMesh : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHIMesh)
    RHI_RESOURCE_IMP(Mesh)
    RHI_RESOURCE_IMP(SkeletalMesh)
};

class IRHITexture : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHITexture)
    RHI_RESOURCE_IMP(Texture)
    virtual void GetSize(int &, int &, int &) const = 0;
    virtual PixelFormat GetPixelFormat() const = 0;
};

class IRHIWindowTarget : public IRHIResource
{
public:
    RHI_TYPE_IMP(IRHIWindowTarget)
    RHI_RESOURCE_IMP(IWindow)
    virtual void Activate() = 0;
    virtual void Swap() = 0;
    virtual void GetSize(int &, int &) const = 0;
    virtual bool Valid() const = 0;
};

class IRHIRenderGraph
{
public:
    virtual ~IRHIRenderGraph() = default;
    virtual void Execute() = 0;
};

class IRHIRenderGraphBuilder
{
public:
    virtual ~IRHIRenderGraphBuilder() = default;
    virtual Ref<IRHIRenderGraph> Build(RenderGraphBuilder *rgb, bool recompile) = 0;
};

#define FABRIC_IMP(X, Y) virtual Ref<IRHI##Y> Gen##X() = 0;
#define FABRIC(X) FABRIC_IMP(X, X)

class RenderGraphBuilder;
class IRHIHelper
{
public:
    static IRHIHelper *Get()
    {
        return GetIRHIHelper();
    }

    FABRIC(Mesh);
    FABRIC(Skeleton);
    FABRIC(Texture);
    FABRIC(WindowTarget);
    FABRIC(RenderGraphBuilder);

    virtual bool Init() = 0;
    virtual bool Inited() = 0;
};

#undef FABRIC
#undef FABRIC_IMP

#endif // __IRHIHELPER_H__