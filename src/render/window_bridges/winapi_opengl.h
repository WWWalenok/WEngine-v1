#ifndef __WINAPI_OPENGL_H__
#define __WINAPI_OPENGL_H__

#include "../opengl/OpenglRHIHelper.h"
#include "../opengl/OpenglRenderGraphBuilder.h"
#include "../../window/winapi.h"
#include "../IRHIBridge.h"
#include <GL/glew.h>

#undef UNICODE
#include <windows.h>
#include <stdio.h>

struct WinapiOpenglBridge : public IRHIBridge {
    struct WinapiOpenglBridgeWindowExt {
        HGLRC hRC;
    };

    virtual int Prepare() override {
        WL_START_TYMETRACE;
        auto wh = dynamic_cast<WINAPIWindowHelper*>(GetIWindowsHelper());
        if (!wh) return 1;
        return wh->ProcessBegin();
    }

    virtual int Finish() override {
        WL_START_TYMETRACE;
        auto wh = dynamic_cast<WINAPIWindowHelper*>(GetIWindowsHelper());
        if (wh) wh->ProcessEnd();
        return 0;
    }

    virtual void OnResize(IWindow* window, int t, int l, int b, int r) override {
        auto* win = dynamic_cast<WINAPIWindow*>(window);
        if (win) {
            win->WIDTH = r; win->HEIGHT = b;
        }
    }

    virtual void OnCreate(IWindow* window) override {
        auto* win = dynamic_cast<WINAPIWindow*>(window);
        if (!win) return;
        if (window->GetExtSize() < sizeof(WinapiOpenglBridgeWindowExt))
            window->SetExtSize(sizeof(WinapiOpenglBridgeWindowExt));

        auto* ext = window->GetExt<WinapiOpenglBridgeWindowExt>();
        ext->hRC = wglCreateContext(win->hDC);
        wglMakeCurrent(win->hDC, ext->hRC);

        glewExperimental = GL_TRUE;
        glewInit();

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0,0,0,0);
    }

    virtual void OnClose(IWindow* window) override {
        auto* ext = window->GetExt<WinapiOpenglBridgeWindowExt>();
        if (ext && ext->hRC) {
            wglDeleteContext(ext->hRC);
        }
    }

    virtual void OnDestroy(IWindow* window) override { OnClose(window); }


    virtual void OnActivate(IWindow* window) override
    {
        WL_START_TYMETRACE;
        auto* win = dynamic_cast<WINAPIWindow*>(window);
        auto* ext = window->GetExt<WinapiOpenglBridgeWindowExt>();
        if (win && ext && ext->hRC) {
            wglMakeCurrent(win->hDC, ext->hRC);
        }
    }

    virtual void OnSwap(IWindow* window) override
    {
        WL_START_TYMETRACE;
        auto* win = dynamic_cast<WINAPIWindow*>(window);
        if (win && win->hDC) {
            SwapBuffers(win->hDC);
        }
    }

    virtual void OnGetSize(const IWindow* window, int& w, int& h) override
    {
        WL_START_TYMETRACE;
        auto* win = dynamic_cast<const WINAPIWindow*>(window);
        if (win) {
            w = win->WIDTH;
            h = win->HEIGHT;
        }
    }
    
};

static IRHIBridge* GetIRHIBridge() {
    static WinapiOpenglBridge bridge;
    return &bridge;
}

#endif