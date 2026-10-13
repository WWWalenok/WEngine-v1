#include "RenderGraphBuilder.h"
#include "../World/World.h"
#include "../World/WObject.h"


void RGBWorldRender::Update() 
{
    const auto& objs = _world->GetObjects();
    size_t hash = 0;
    for (auto obj : objs)
    {
        hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)obj.get());
        for (auto child : obj->GetAllChildren()) 
        {
            hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)child.get());
        }
        for (auto comp : obj->GetAllComponents()) 
        {
            hash = ((hash << 31) | (hash >> 33)) ^ ((size_t)comp.get());
        }
    }
    if(oldhash != hash)
        SetDirty();
    oldhash = hash;
}