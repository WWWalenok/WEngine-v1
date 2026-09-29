#ifndef __WINAPI_H__
#define __WINAPI_H__

#include "../core/WLoger/include/WLoger.h"
#include "../core/core.h"

#if WLOG_OS_WINDOWS

#include "../render/IWindowHelper.h"
#include "../input/IInputHelper.h"
#include "../input/IInputBridge.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <list>
#include <mutex>
#include <array>

#undef UNICODE
#include <windows.h>
#include <stdio.h>

// ============================================================================
//  Клавиатура
// ============================================================================
struct WINAPIKeyboard : public IKeyboard
{
    std::array<bool, (size_t)KeyCode::Count> down{};
    std::array<bool, (size_t)KeyCode::Count> pressed{};
    std::array<bool, (size_t)KeyCode::Count> released{};

    bool IsKeyDown(KeyCode k) const override;
    bool IsKeyUp(KeyCode k) const override;
    bool IsKeyPressed(KeyCode k) const override;
    bool IsKeyReleased(KeyCode k) const override;

    void Update() override;
    void ResetFrameState() override;
    void ResetAll();

    void OnKeyDown(WPARAM vk);
    void OnKeyUp(WPARAM vk);
};

// ============================================================================
//  Мышь
// ============================================================================
struct WINAPIMouse : public IMouse
{
    std::array<bool, (size_t)MouseButton::Count> down{};
    std::array<bool, (size_t)MouseButton::Count> pressed{};
    std::array<bool, (size_t)MouseButton::Count> released{};

    int posX = 0, posY = 0;
    int frameDeltaX = 0, frameDeltaY = 0;
    int pendingDeltaX = 0, pendingDeltaY = 0;
    float frameScroll = 0.0f;
    float pendingScroll = 0.0f;

    bool cursorVisible = true;
    bool cursorLocked  = false;

    bool IsButtonDown(MouseButton b) const override;
    bool IsButtonPressed(MouseButton b) const override;
    bool IsButtonReleased(MouseButton b) const override;

    void GetPosition(int& x, int& y) const override;
    void GetDelta(int& dx, int& dy) const override;
    float GetScroll() const override;

    void SetCursorVisible(bool v) override;
    bool IsCursorVisible() const override;
    void SetCursorLocked(bool v) override;
    bool IsCursorLocked() const override;

    void Update() override;
    void ResetFrameState() override;

    void OnMove(int x, int y);
    void OnButtonDown(MouseButton b);
    void OnButtonUp(MouseButton b);
    void OnScroll(float d);
    void CommitFrame();
};

// ============================================================================
//  Геймпад
// ============================================================================
struct WINAPIGamepad : public IGamepad
{
    int slot = 0;
    bool connected = false;

    std::array<bool,  (size_t)GamepadButton::Count> down{};
    std::array<bool,  (size_t)GamepadButton::Count> pressed{};
    std::array<bool,  (size_t)GamepadButton::Count> released{};
    std::array<float, (size_t)GamepadAxis::Count>   axes{};

    int  GetSlot() const override;
    bool IsConnected() const override;
    bool IsButtonDown(GamepadButton b) const override;
    bool IsButtonPressed(GamepadButton b) const override;
    bool IsButtonReleased(GamepadButton b) const override;
    float GetAxis(GamepadAxis a) const override;

    void Update() override;
    void ResetFrameState() override;
};

// ============================================================================
//  Input Helper
// ============================================================================
struct WINAPIInputHelper : public IInputHelper
{
    static WINAPIInputHelper& Get();

    bool inited = false;

    Ref<WINAPIKeyboard> keyboard;
    Ref<WINAPIMouse>    mouse;
    Ref<WINAPIGamepad>  gamepads[MAX_GAMEPADS];
    int                 gamepadCount = 0;

    bool Init() override;
    bool Inited() override;
    void Shutdown() override;

    Ref<IKeyboard> GetKeyboard() override;
    Ref<IMouse>    GetMouse() override;
    Ref<IGamepad>  GetGamepad(int slot) override;
    int            GetGamepadCount() override;
};

// ============================================================================
//  Input Bridge
// ============================================================================
struct WINAPIInputBridge : public IInputBridge
{
    static WINAPIInputBridge& Get();

    int  Prepare() override;
    int  Finish() override;
    void Reset() override;

    void HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

void WinAPIInputDispatchMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// ============================================================================
//  Window
// ============================================================================
struct WINAPIWindow : public IWindow
{
    HDC  hDC  = nullptr;
    HWND hWnd = nullptr;

    int WIDTH  = 0;
    int HEIGHT = 0;

    std::string szAppName;
    WNDCLASS    wndclass{};

    EventHandler* eh = nullptr;

    int make(std::string name, int w = 512, int h = 512);

    void SetEventHandler(EventHandler* _) override;

    void WHOnCreate(WPARAM wParam, LPARAM lParam);
    void WHOnPaint(WPARAM wParam, LPARAM lParam);
    void WHOnResize(WPARAM wParam, LPARAM lParam);
    void WHOnDestroy(WPARAM wParam, LPARAM lParam);

    std::vector<uint8_t> ext;

    void   SetExtSize(size_t _) override;
    size_t GetExtSize() override;
    void*  GetExtImp() override;

    void Activate() override;
    void Swap() override;
    void GetSize(int& w, int& h) const override;
    bool Valid() const override;
};

// ============================================================================
//  Window Helper
// ============================================================================
struct WINAPIWindowHelper : public IWindowHelper
{
    static WINAPIWindowHelper& Get();

    std::mutex winmutex;

    Ref<IWindow> MakeWindow(IWindow::EventHandler* eh,
                            std::string name,
                            int WIDTH = 512, int HEIGHT = 512) override;

    std::vector<IWindow*> cashed;

    IWindow** GetWindows() override;
    int       GetWindowsCount() override;

    int ProcessBegin();
    int ProcessEnd();
};

#endif // WLOG_OS_WINDOWS
#endif // __WINAPI_H__