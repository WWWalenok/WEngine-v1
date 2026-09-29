#ifndef __IWINDOWHELPER_H__
#define __IWINDOWHELPER_H__

#include "../core/core.h"
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
        virtual void OnActivate(IWindow*) {};
        virtual void OnSwap(IWindow*) {};
        virtual void OnGetSize(const IWindow*, int& width, int& height) {};
    };

    virtual void SetEventHandler(EventHandler*) = 0;

    virtual void SetExtSize(size_t) = 0;
    virtual size_t GetExtSize() = 0;
    virtual void* GetExtImp() = 0;
    template<typename T>
    T* GetExt() { return GetExtSize() < sizeof(T) ? nullptr : (T*)GetExtImp(); }

    virtual void Activate() = 0;
    virtual void Swap() = 0;
    virtual void GetSize(int& width, int& height) const = 0;

    virtual bool Valid() const = 0;
};

struct IWindowHelper
{
    virtual Ref<IWindow> MakeWindow(IWindow::EventHandler*, std::string, int = 512, int = 512) = 0;

    virtual IWindow** GetWindows() = 0;
    virtual int GetWindowsCount() = 0;
};

IWindowHelper* GetIWindowsHelper();

#endif // __IWINDOWHELPER_H__