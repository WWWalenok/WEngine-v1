#pragma once


#include "../core/CoreSystem.h"
#include "window_helper/IWindowHelper.h"
#include "window_bridges/IRHIBridge.h"
#include "rhi_helpers/IRHIHelper.h"

struct WWorld;
struct UI;

class RenderSystem : public ISystem
{
    DECLARE_SYSTEM(RenderSystem);
    RenderSystem() :
        thread(0, true)
    {
        rhi = GetIRHIHelper();
        wh = GetIWindowsHelper();
        bridge = GetIRHIBridge();
    }

public:
    ThredaPool thread;
    IRHIHelper* rhi;
    IWindowHelper* wh;
    IRHIBridge* bridge;

    WWorld* currnt_world;
    UI* currnt_ui;

    Ref<IRHIMesh> GenMesh() { return rhi->GenMesh(); }
    Ref<IRHISkeleton> GenSkeleton() { return rhi->GenSkeleton(); }

    IWindow* MakeWindow(std::string name, int w = 512, int h = 512) { 
        return wh->MakeWindow(bridge, name, w, h); 
    }
    
    void Start() {
        for(int i = 0; i < 10; ++i)
            bridge->Render(&thread, nullptr, nullptr);
    }

    int Process()
    {
        if(wh && rhi && bridge)
        {
            thread.Process();
            auto ret = bridge->Render(&thread, currnt_world, currnt_ui);
            if(ret != 0)
                return ret;
        }
        else
            return 1;
        return 0;
    }

};