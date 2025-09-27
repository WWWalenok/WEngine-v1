
#pragma once

#include "IWindowHelper.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#define WIN32_LEAN_AND_MEAN
#undef UNICODE
#include <windows.h>
#include <stdio.h>

static LONG WINAPI WindowHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct WINAPIWindow : public IWindow
{
    HDC  hDC;
    HWND hWnd;

    int WIDTH;
    int HEIGHT;

    std::string szAppName;

    WNDCLASS wndclass;

    int make(std::string name, int w = 512, int h = 512)
    {
        szAppName = name;
        WIDTH = w;
        HEIGHT = h;
        auto     hInstance = GetModuleHandleW(NULL);
        /* Register the frame class */
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = (WNDPROC)WindowHandler;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = hInstance;
        wndclass.hIcon         = LoadIcon(hInstance, szAppName.c_str());
        wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wndclass.lpszMenuName  = szAppName.c_str();
        wndclass.lpszClassName = szAppName.c_str();

        if (!RegisterClass(&wndclass))
            return 2;
        /* Create the frame */
        hWnd = CreateWindow(szAppName.c_str(), "Generic OpenGL Sample", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
        if (!hWnd)
            return 1;

        /* show and update main window */
        ShowWindow(hWnd, SW_NORMAL);

        UpdateWindow(hWnd);
        
        MSG msg;

        if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                return 1;
            }
        }
        return 0;
    }
    

    EventHandler* eh = nullptr;

    virtual void SetEventHandler(EventHandler* _)
    {
        eh = _;
    }

    void WHOnCreate(WPARAM wParam, LPARAM lParam)
    {
        hDC = GetDC(hWnd);
        PIXELFORMATDESCRIPTOR pfd, *ppfd;
        int                   pixelformat;

        ppfd = &pfd;

        ppfd->nSize        = sizeof(PIXELFORMATDESCRIPTOR);
        ppfd->nVersion     = 1;
        ppfd->dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        ppfd->dwLayerMask  = PFD_MAIN_PLANE;
        ppfd->iPixelType   = PFD_TYPE_COLORINDEX;
        ppfd->cColorBits   = 8;
        ppfd->cDepthBits   = 16;
        ppfd->cAccumBits   = 0;
        ppfd->cStencilBits = 0;

        pixelformat = ChoosePixelFormat(hDC, ppfd);

        if ((pixelformat = ChoosePixelFormat(hDC, ppfd)) == 0)
        {
            return;
        }

        if (SetPixelFormat(hDC, pixelformat, ppfd) == FALSE)
        {
            return;
        }

        if(eh) eh->OnCreate(this);
    }

    void WHOnPaint(WPARAM wParam, LPARAM lParam)
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        if(eh) eh->OnPaint(this);
        EndPaint(hWnd, &ps);
    }

    void WHOnResize(WPARAM wParam, LPARAM lParam)
    {
        RECT rect;
        GetClientRect(hWnd, &rect);
        float aspect;
        WIDTH  = rect.right;
        HEIGHT = rect.bottom;
        if(eh) eh->OnResize(this, rect.top, rect.left, rect.bottom, rect.right);
    }
    
    std::vector<uint8_t> ext;

    virtual void SetExtSize(size_t _) { ext.resize(_); }
    virtual size_t GetExtSize() { return ext.size(); }
    virtual void* GetExtImp() { return ext.data(); }

};

struct WINAPIWindowHelper : public IWindowHelper
{
    static WINAPIWindowHelper& Get() {
        static WINAPIWindowHelper _; return _;
    }

    static std::unordered_map<HWND, WINAPIWindow*>& winmap()
    {
        static std::unordered_map<HWND, WINAPIWindow*> _;
        return _;
    };

    static std::vector<WINAPIWindow>& winvec()
    {
        static std::vector<WINAPIWindow> _;
        return _;
    };

    int ProcessBegin() 
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE) == TRUE)
        {
            if (GetMessage(&msg, NULL, 0, 0))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                return 1;
            }
        }
        return 0;
    }

    int ProcessEnd() 
    {
        std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
        auto& vec = WINAPIWindowHelper::winvec();
        for(auto& el: vec)
            SwapBuffers(el.hDC);
        return 0;
    }


    std::mutex winmutex;

    virtual IWindow* MakeWindow(IWindow::EventHandler* eh, std::string name, int WIDTH = 512, int HEIGHT = 512)
    {
        // std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
        auto& map = winmap();
        auto& vec = winvec();
        vec.push_back(WINAPIWindow());
        auto& back = vec.back();
        back.SetEventHandler(eh);
        back.make(name, WIDTH, HEIGHT);
        map[back.hWnd] = &back;
        
        return 0;
    }

    std::vector<WINAPIWindow> cashed;

    virtual IWindow* GetWindows() {
        std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
        cashed = winvec();

        return cashed.data();
    }
    virtual int GetWindowsCount() {
        std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
        return winvec().size();
    }
};

static LONG WINAPI WindowHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WINAPIWindowHelper::Get().winmutex.lock();
    auto& map = WINAPIWindowHelper::winmap();
    auto& vec = WINAPIWindowHelper::winvec();
    WINAPIWindow* owner = vec.size() > 0 ? (vec.back().hWnd == nullptr ? &vec.back() : nullptr) : nullptr;
    if (map.count(hWnd) != 0)
        owner = map.at(hWnd);
    WINAPIWindowHelper::Get().winmutex.unlock();


    if(!owner)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    LONG lRet = 1;

    RECT rect;
    
    switch (uMsg)
    {

        case WM_CREATE:

            if(owner->hWnd != hWnd)
            {
                owner->hWnd = hWnd;
            }
            owner->WHOnCreate(wParam, lParam);
            break;

        case WM_PAINT:
            if (owner)
                owner->WHOnPaint(wParam, lParam);
            break;

        case WM_SIZE:
            owner->WHOnResize(wParam, lParam);
            break;

        case WM_CLOSE:
            if (owner->hDC)
                ReleaseDC(hWnd, owner->hDC);
            owner->hDC = 0;

            DestroyWindow(hWnd);
            break;

        case WM_DESTROY:
            if (owner->hDC)
                ReleaseDC(hWnd, owner->hDC);
            PostQuitMessage(0);
            break;

        case WM_KEYDOWN:
            break;

        default:
            lRet = DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;
    }

    return lRet;
}

IWindowHelper* GetIWindowsHelper() { return &WINAPIWindowHelper::Get(); }