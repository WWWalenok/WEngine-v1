
#pragma once
#include <string>


struct IWindow
{
    struct EventHandler
    {
        virtual void OnPaint(IWindow*) {};
        virtual void OnCreate(IWindow*) {};
        virtual void OnResize(IWindow*, int t, int l, int b, int r) {};
        virtual void OnKeyDown(IWindow*, void* wParam) {};
        virtual void OnDestroy(IWindow*) {};
        virtual void OnClose(IWindow*) {};
    };

    virtual void SetEventHandler(EventHandler*) = 0;

    virtual void SetExtSize(size_t) = 0;
    virtual size_t GetExtSize() = 0;
    virtual void* GetExtImp() = 0;
    template<typename T>
    T* GetExt() { return GetExtSize() < sizeof(T) ? nullptr : (T*)GetExtImp(); }
};

struct IWindowHelper
{
    virtual IWindow* MakeWindow(IWindow::EventHandler*, std::string, int = 512, int = 512) = 0;

    virtual IWindow* GetWindows() = 0;
    virtual int GetWindowsCount() = 0;
};

static IWindowHelper* GetIWindowsHelper();
