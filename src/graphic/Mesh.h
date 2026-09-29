#pragma once
#include "../core/core.h"
#include "../render/IRHIHelper.h"
#include <string>
#include "Material.h"

struct Mesh : public CoreObject 
{
    struct Vertex
    {
        MVector3f v;
        MVector3f vt;
        MVector3f vn;
    };

    std::vector<Vertex>   v;
    std::vector<uint32_t> f;

    std::string name;

    Ref<Material> material = nullptr;
    Ref<IRHIMesh> rhimesh  = nullptr;

    void Update() {
        if (!rhimesh) {
            rhimesh = IRHIHelper::Get()->GenMesh();
        }
        rhimesh->Update(this);
    }
};

struct Skeleton
{

    struct Bone
    {
        WeakRef<Skeleton>    skeleton;
        int              parent = -1;
        int              id;
        std::vector<int> childs;
    };

    std::vector<MMatrix4f> boneMatrices;
    std::vector<Bone> bones;
    Ref<IRHISkeleton> rhiskeleton = nullptr;

    void Update() {
        if (!rhiskeleton) {
            rhiskeleton = IRHIHelper::Get()->GenSkeleton();
        }
        rhiskeleton->Update(this);
    }
};


struct SkeletalMesh
{
    struct Vertex
    {
        MVector3f         v;
        MVector3f         vt;
        MVector3f         vn;
        MVector<int, 8>   bi;
        MVector<float, 8> bv;
    };

    std::vector<Vertex>   v;
    std::vector<uint32_t> f;

    std::string name;

    Ref<Material> material = nullptr;
    Ref<IRHIMesh> rhimesh  = nullptr;

    void Update() {
        if (!rhimesh) {
            rhimesh = IRHIHelper::Get()->GenMesh();
        }
        rhimesh->Update(this);
    }
};

