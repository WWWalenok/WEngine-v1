#ifndef __RENDERSYSTEM_H__
#define __RENDERSYSTEM_H__

#include "../core/CoreSystem.h"
#include "IWindowHelper.h"
#include "IRHIBridge.h"
#include "IRHIHelper.h"
#include "RenderGraphBuilder.h"

struct World;
struct UI;
struct Camera;

class RenderSystem : public ISystem {
    DECLARE_SYSTEM(RenderSystem);
    RenderSystem() : thread(0, true) {
        rhi = GetIRHIHelper();
        wh = GetIWindowsHelper();
        bridge = GetIRHIBridge();
    }

public:
    ThreadPool thread;
    IRHIHelper* rhi;
    IWindowHelper* wh;
    IRHIBridge* bridge;
    std::list<Ref<RenderGraphBuilder>> RGBS;

    Ref<IRHIMesh> GenMesh() { return rhi->GenMesh(); }
    Ref<IRHISkeleton> GenSkeleton() { return rhi->GenSkeleton(); }
    Ref<IWindow> MakeWindow(std::string name, int w=512, int h=512) {
        return wh->MakeWindow(bridge, name, w, h);
    }

    void AddRGB(Ref<RenderGraphBuilder> RGB) {
        RGBS.push_back(RGB);
    }

    void Start() {
        rhi->Init();
        for(int i=0; i<10; ++i) {
            bridge->Prepare();
            bridge->Finish();
        }
    }


    int Process() {
        if (!wh || !rhi || !bridge) return 1;
        WL_START_TYMETRACE;

        bridge->Prepare();
        thread.Process();

        bool is_any_processed = false;

        for (auto& rgb : RGBS) {
            WL_START_CUSTOM_TYMETRACE(for (auto& rgb : RGBS));
            IRHIRenderGraph* graph = rgb->Build();

            auto root = rgb->Root().get();
            if (root->GetType() ==  GID(RGBWindowOutputNode)) {
                WL_START_CUSTOM_TYMETRACE(if (root->GetType() ==  GID(RGBWindowOutputNode)));
                auto winOut = static_cast<RGBWindowOutputNode*>(root);
                if(!winOut->windowTarget->Valid())
                    continue;
                winOut->windowTarget->Activate();
            }
            graph->Execute();
            is_any_processed = true;
        }

        bridge->Finish();
        return is_any_processed ? 0 : 1;
    }
};

#endif