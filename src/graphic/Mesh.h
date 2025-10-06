#pragma once
#include "../core/core.h"
#include "../render/rhi_helpers/IRHIHelper.h"
#include <string>
#include "Material.h"

struct Mesh
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
};
DECLARE_DATA_TYPE(Mesh);

struct Skeleton
{

    struct Bone
    {
        Ref<Skeleton>    skeleton;
        int              parent = -1;
        int              id;
        std::vector<int> childs;
    };

    std::vector<MMatrix4f> boneMatrices;
    std::vector<Bone> bones;
    Ref<IRHISkeleton> rhiskeleton = nullptr;
};

DECLARE_DATA_TYPE(Skeleton);

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
};

DECLARE_DATA_TYPE(SkeletalMesh);
