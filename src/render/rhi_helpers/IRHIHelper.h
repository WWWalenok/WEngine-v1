#pragma once


class IRHIHelper;
static IRHIHelper* GetIRHIHelper();

struct WWorld;
struct UI;
struct Skeleton;
struct Mesh;
struct SkeletalMesh;
struct ProceduralTexture;

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

    UNDEFINED // Count of types
};
    

class IRHISkeleton {
public:
    virtual void Update(Ref<Skeleton>) = 0;
};
DECLARE_DATA_TYPE(IRHISkeleton);

class IRHIMesh {
public:
    virtual void Update(Ref<Mesh>) = 0;
    virtual void Update(Ref<SkeletalMesh>) = 0;
};
DECLARE_DATA_TYPE(IRHIMesh);

class IRHITexture {
public:
    
    virtual void Update(Ref<Texture>) = 0;
    virtual PixelFormat GetPixelFormat() const = 0;
    virtual size_t GetWidth() const = 0;
    virtual size_t GetHeight() const = 0;
};
DECLARE_DATA_TYPE(IRHITexture);

class IRHIRenderTarget {
public:
    virtual void Update(Ref<RenderTarget>) = 0;
    virtual PixelFormat GetPixelFormat() const = 0;
};
DECLARE_DATA_TYPE(IRHIRenderTarget);

class IRHIProceduralTexture {
public:
    virtual void Update(Ref<ProceduralTexture>) = 0;
    virtual PixelFormat GetPixelFormat() const = 0;
};
DECLARE_DATA_TYPE(IRHIProceduralTexture);


#define FABRIC(X) virtual Ref<IRHI ## X> Gen ## X() = 0;

class IRHIHelper {
public:
    static IRHIHelper* Get()
    {
        return GetIRHIHelper();
    }

    FABRIC(Mesh);
    FABRIC(Skeleton);
    FABRIC(Texture);
    FABRIC(RenderTarget);
    FABRIC(ProceduralTexture);

    virtual bool Inited() = 0;

    virtual int RenderWorld(WWorld*) = 0;
    virtual int RenderMesh(Ref<IRHIMesh>) = 0;
    virtual int RenderUI(UI*) { return 0; };
};

#undef FABRIC(X)