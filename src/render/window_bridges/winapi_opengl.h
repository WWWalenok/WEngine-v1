#pragma once

#include "../rhi_helpers/opengl.h"
#include "../window_helper/winapi.h"

struct WinapiOpenglBridge : public IRHIBridge
{
    struct WinapiOpenglBridgeWindowExt
    {
        HGLRC hRC;
    };

    int Render(WINAPIWindow* wh, WWorld* world, UI* ui)
    {
        auto rhi = dynamic_cast<OpenglRHIHelper*>(GetIRHIHelper());
        if(!wh)
            return 1;
        int  ret;
        auto ext = wh->GetExt<WinapiOpenglBridgeWindowExt>();
        rhi->WIDTH = wh->WIDTH;
        rhi->HEIGHT = wh->HEIGHT;
        ret = rhi->RenderWorld(world);
        if(ret != 0)
            return ret;

        ret = rhi->RenderUI(ui);
        return ret;
    }

    virtual int Prepare()
    {
        auto rhi = dynamic_cast<OpenglRHIHelper*>(GetIRHIHelper());
        auto wh  = dynamic_cast<WINAPIWindowHelper*>(GetIWindowsHelper());
        if(!rhi || !wh)
            sthrow std::runtime_error("RHI or Window manager incorrect.")
        int  ret = wh->ProcessBegin();
        if(ret != 0)
            return ret;
    }

    virtual int Render(WWorld* world, UI* ui) 
    {

        for(auto& window: WINAPIWindowHelper::winvec())
        {
            auto ext = window.GetExt<WinapiOpenglBridgeWindowExt>();
            if(ext && (world || ui))
            { 
                Render(&window, world, ui);
            }
        }
    }

    virtual int Render(std::unordered_map<IWindow*, WWorld*> _world, std::unordered_map<IWindow*, UI*> _ui) 
    {
        WWorld* world;
        UI* ui;

        for(auto& window: WINAPIWindowHelper::winvec())
        {
            auto ext = window.GetExt<WinapiOpenglBridgeWindowExt>();
            world = _world.count(&window) ? _world.at(&window) : nullptr;
            ui = _ui.count(&window) ? _ui.at(&window) : nullptr;
            if(ext && (world || ui))
            { 
                Render(&window, world, ui);
            }
        }
    }
    
    virtual int Finish()
    {
        wh->ProcessEnd();
    }

    virtual void OnResize(IWindow* window, int t, int l, int b, int r)
    {
        auto rhi = dynamic_cast<OpenglRHIHelper*>(GetIRHIHelper());
        auto wh  = dynamic_cast<WINAPIWindow*>(window);
        if (rhi && wh)
        {
            wh->WIDTH  = r;
            wh->HEIGHT = b;
            auto ext = wh->GetExt<WinapiOpenglBridgeWindowExt>();

            wglMakeCurrent(wh->hDC, ext->hRC);

            glViewport(0, 0, r, b);
            // Select the projection stack and apply
            // an orthographic projection
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0.0, r, b, 0.0, -1.0, 1.0);
            glMatrixMode(GL_MODELVIEW);
        }
    }
    
    virtual void OnCreate(IWindow* window)
    {
        auto rhi = dynamic_cast<OpenglRHIHelper*>(GetIRHIHelper());
        auto wh  = dynamic_cast<WINAPIWindow*>(window);
        if (rhi && wh)
        {
            if(window->GetExtSize() < sizeof(WinapiOpenglBridgeWindowExt))
                window->SetExtSize(sizeof(WinapiOpenglBridgeWindowExt));

            auto ext = window->GetExt<WinapiOpenglBridgeWindowExt>();

            if(!ext)
                throw std::runtime_error("Window Extention block to small");

            auto hDC  = wh->hDC;
            auto hWnd = wh->hWnd;

            RECT rect;
            ext->hRC = wglCreateContext(hDC);

            rhi->inited = true;

            wglMakeCurrent(hDC, ext->hRC);
            GetClientRect(hWnd, &rect);

            glewExperimental = GL_TRUE;
            if (glewInit() != GLEW_OK)
            {
                return;
            }

            GLfloat  maxObjectSize, aspect;
            GLdouble near_plane, far_plane;

            glEnable(GL_TEXTURE_2D);
            // Choose a smooth shading model
            glShadeModel(GL_SMOOTH);
            // Set the clear color to black
            glClearColor(0.0, 0.0, 0.0, 0.0);

            glEnable(GL_ALPHA_TEST);
            glAlphaFunc(GL_GREATER, 0.0f);

            glDisable(GL_DEPTH_TEST);
        }
    };
    
    virtual void OnClose(IWindow* window)
    {
        auto rhi = dynamic_cast<OpenglRHIHelper*>(GetIRHIHelper());
        auto wh  = dynamic_cast<WINAPIWindow*>(window);
        if (rhi && wh)
        {
            auto hDC  = wh->hDC;
            auto hWnd = wh->hWnd;
            auto ext = window->GetExt<WinapiOpenglBridgeWindowExt>();
            if(ext)
            {
                if(ext->hRC)
                    wglDeleteContext(ext->hRC);
                delete ext;
            }
        }
    }
    
    virtual void OnDesroy(IWindow* window)
    {
        OnClose(window);
    }
};

static IRHIBridge* GetIRHIBridge()
{
    static WinapiOpenglBridge _;
    return &_;
}