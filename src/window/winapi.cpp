#include "winapi.h"

#if WLOG_OS_WINDOWS

#include <windowsx.h>

// ============================================================================
//  Внутренние статические хранилища (приватные для этого .cpp)
// ============================================================================
static LONG WINAPI WindowHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static std::unordered_map<HWND, WINAPIWindow*>& winmap()
{
    static std::unordered_map<HWND, WINAPIWindow*> _;
    return _;
}

static std::list<Ref<WINAPIWindow>>& winlist()
{
    static std::list<Ref<WINAPIWindow>> _;
    return _;
}

// ============================================================================
//  Маппинг VK -> KeyCode
// ============================================================================
static KeyCode VKToKeyCode(WPARAM vk)
{
    if (vk >= 'A' && vk <= 'Z')
        return (KeyCode)((int)KeyCode::A + (vk - 'A'));
    if (vk >= '0' && vk <= '9')
        return (KeyCode)((int)KeyCode::Num0 + (vk - '0'));

    switch (vk)
    {
        case VK_ESCAPE:   return KeyCode::Escape;
        case VK_RETURN:   return KeyCode::Enter;
        case VK_SPACE:    return KeyCode::Space;
        case VK_TAB:      return KeyCode::Tab;
        case VK_BACK:     return KeyCode::Backspace;
        case VK_LSHIFT:   return KeyCode::LeftShift;
        case VK_RSHIFT:   return KeyCode::RightShift;
        case VK_LCONTROL: return KeyCode::LeftCtrl;
        case VK_RCONTROL: return KeyCode::RightCtrl;
        case VK_LMENU:    return KeyCode::LeftAlt;
        case VK_RMENU:    return KeyCode::RightAlt;
        case VK_UP:       return KeyCode::Up;
        case VK_DOWN:     return KeyCode::Down;
        case VK_LEFT:     return KeyCode::Left;
        case VK_RIGHT:    return KeyCode::Right;
        case VK_F1:  return KeyCode::F1;  case VK_F2:  return KeyCode::F2;
        case VK_F3:  return KeyCode::F3;  case VK_F4:  return KeyCode::F4;
        case VK_F5:  return KeyCode::F5;  case VK_F6:  return KeyCode::F6;
        case VK_F7:  return KeyCode::F7;  case VK_F8:  return KeyCode::F8;
        case VK_F9:  return KeyCode::F9;  case VK_F10: return KeyCode::F10;
        case VK_F11: return KeyCode::F11; case VK_F12: return KeyCode::F12;
    }
    return KeyCode::Unknown;
}

// ============================================================================
//  WINAPIKeyboard
// ============================================================================
bool WINAPIKeyboard::IsKeyDown(KeyCode k) const     { return down[(size_t)k]; }
bool WINAPIKeyboard::IsKeyUp(KeyCode k) const       { return !down[(size_t)k]; }
bool WINAPIKeyboard::IsKeyPressed(KeyCode k) const  { return pressed[(size_t)k]; }
bool WINAPIKeyboard::IsKeyReleased(KeyCode k) const { return released[(size_t)k]; }

void WINAPIKeyboard::Update() {}

void WINAPIKeyboard::ResetFrameState()
{
    pressed.fill(false);
    released.fill(false);
}

void WINAPIKeyboard::ResetAll()
{
    down.fill(false);
    pressed.fill(false);
    released.fill(false);
}

void WINAPIKeyboard::OnKeyDown(WPARAM vk)
{
    auto k = VKToKeyCode(vk);
    if (k == KeyCode::Unknown)
        return;
    auto i = (size_t)k;
    if (!down[i])
        pressed[i] = true;
    down[i] = true;
}

void WINAPIKeyboard::OnKeyUp(WPARAM vk)
{
    auto k = VKToKeyCode(vk);
    if (k == KeyCode::Unknown)
        return;
    auto i = (size_t)k;
    if (down[i])
        released[i] = true;
    down[i] = false;
}

// ============================================================================
//  WINAPIMouse
// ============================================================================
bool WINAPIMouse::IsButtonDown(MouseButton b) const     { return down[(size_t)b]; }
bool WINAPIMouse::IsButtonPressed(MouseButton b) const  { return pressed[(size_t)b]; }
bool WINAPIMouse::IsButtonReleased(MouseButton b) const { return released[(size_t)b]; }

void WINAPIMouse::GetPosition(int& x, int& y) const { x = posX; y = posY; }
void WINAPIMouse::GetDelta(int& dx, int& dy)  const { dx = frameDeltaX; dy = frameDeltaY; }
float WINAPIMouse::GetScroll() const                { return frameScroll; }

void WINAPIMouse::SetCursorVisible(bool v)
{
    if (v) { while (ShowCursor(TRUE)  < 0)  {} }
    else   { while (ShowCursor(FALSE) >= 0) {} }
    cursorVisible = v;
}

bool WINAPIMouse::IsCursorVisible() const { return cursorVisible; }
void WINAPIMouse::SetCursorLocked(bool v) { cursorLocked = v; }
bool WINAPIMouse::IsCursorLocked() const  { return cursorLocked; }

void WINAPIMouse::Update() {}

void WINAPIMouse::ResetFrameState()
{
    pressed.fill(false);
    released.fill(false);
    frameDeltaX = frameDeltaY = 0;
    frameScroll = 0.0f;
    pendingDeltaX = pendingDeltaY = 0;
    pendingScroll = 0.0f;
}

void WINAPIMouse::OnMove(int x, int y)
{
    pendingDeltaX += x - posX;
    pendingDeltaY += y - posY;
    posX = x;
    posY = y;
}

void WINAPIMouse::OnButtonDown(MouseButton b)
{
    auto i = (size_t)b;
    if (!down[i])
        pressed[i] = true;
    down[i] = true;
}

void WINAPIMouse::OnButtonUp(MouseButton b)
{
    auto i = (size_t)b;
    if (down[i])
        released[i] = true;
    down[i] = false;
}

void WINAPIMouse::OnScroll(float d) { pendingScroll += d; }

void WINAPIMouse::CommitFrame()
{
    frameDeltaX = pendingDeltaX;
    frameDeltaY = pendingDeltaY;
    frameScroll = pendingScroll;
    pendingDeltaX = pendingDeltaY = 0;
    pendingScroll = 0.0f;
}

// ============================================================================
//  WINAPIGamepad
// ============================================================================
int  WINAPIGamepad::GetSlot() const { return slot; }
bool WINAPIGamepad::IsConnected() const { return connected; }

bool WINAPIGamepad::IsButtonDown(GamepadButton b) const     { return down[(size_t)b]; }
bool WINAPIGamepad::IsButtonPressed(GamepadButton b) const  { return pressed[(size_t)b]; }
bool WINAPIGamepad::IsButtonReleased(GamepadButton b) const { return released[(size_t)b]; }
float WINAPIGamepad::GetAxis(GamepadAxis a) const           { return axes[(size_t)a]; }

void WINAPIGamepad::Update() { /* XInputGetState здесь */ }

void WINAPIGamepad::ResetFrameState()
{
    pressed.fill(false);
    released.fill(false);
}

// ============================================================================
//  WINAPIInputHelper
// ============================================================================
WINAPIInputHelper& WINAPIInputHelper::Get()
{
    static WINAPIInputHelper _;
    return _;
}

bool WINAPIInputHelper::Init()
{
    if (inited)
        return true;

    keyboard = MakeRef<WINAPIKeyboard>();
    mouse    = MakeRef<WINAPIMouse>();

    for (int i = 0; i < MAX_GAMEPADS; ++i)
    {
        gamepads[i] = MakeRef<WINAPIGamepad>();
        gamepads[i]->slot = i;
    }
    gamepadCount = MAX_GAMEPADS;

    inited = true;
    return true;
}

bool WINAPIInputHelper::Inited() { return inited; }

void WINAPIInputHelper::Shutdown()
{
    keyboard.reset();
    mouse.reset();
    for (auto& gp : gamepads)
        gp.reset();
    gamepadCount = 0;
    inited = false;
}

Ref<IKeyboard> WINAPIInputHelper::GetKeyboard() { return keyboard; }
Ref<IMouse>    WINAPIInputHelper::GetMouse()    { return mouse; }

Ref<IGamepad> WINAPIInputHelper::GetGamepad(int slot)
{
    if (slot < 0 || slot >= MAX_GAMEPADS)
        return nullptr;
    return gamepads[slot];
}

int WINAPIInputHelper::GetGamepadCount() { return gamepadCount; }

// ============================================================================
//  WINAPIInputBridge
// ============================================================================
WINAPIInputBridge& WINAPIInputBridge::Get()
{
    static WINAPIInputBridge _;
    return _;
}

int WINAPIInputBridge::Prepare()
{
    auto& h = WINAPIInputHelper::Get();
    if (h.mouse)
        h.mouse->CommitFrame();
    return 0;
}

int WINAPIInputBridge::Finish()
{
    auto& h = WINAPIInputHelper::Get();
    if (h.keyboard)
        h.keyboard->ResetFrameState();
    if (h.mouse)
        h.mouse->ResetFrameState();
    for (auto& gp : h.gamepads)
        if (gp)
            gp->ResetFrameState();
    return 0;
}

void WINAPIInputBridge::Reset()
{
    auto& h = WINAPIInputHelper::Get();
    if (h.keyboard)
        h.keyboard->ResetAll();
    if (h.mouse)
        h.mouse->ResetFrameState();
}

void WINAPIInputBridge::HandleMessage(HWND /*hWnd*/, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto& h = WINAPIInputHelper::Get();

    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (h.keyboard) h.keyboard->OnKeyDown(wParam);
            break;

        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (h.keyboard) h.keyboard->OnKeyUp(wParam);
            break;

        case WM_MOUSEMOVE:
            if (h.mouse)
                h.mouse->OnMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;

        case WM_LBUTTONDOWN: if (h.mouse) h.mouse->OnButtonDown(MouseButton::Left);   break;
        case WM_LBUTTONUP:   if (h.mouse) h.mouse->OnButtonUp(MouseButton::Left);     break;
        case WM_RBUTTONDOWN: if (h.mouse) h.mouse->OnButtonDown(MouseButton::Right);  break;
        case WM_RBUTTONUP:   if (h.mouse) h.mouse->OnButtonUp(MouseButton::Right);    break;
        case WM_MBUTTONDOWN: if (h.mouse) h.mouse->OnButtonDown(MouseButton::Middle); break;
        case WM_MBUTTONUP:   if (h.mouse) h.mouse->OnButtonUp(MouseButton::Middle);   break;

        case WM_MOUSEWHEEL:
            if (h.mouse)
            {
                short delta = GET_WHEEL_DELTA_WPARAM(wParam);
                h.mouse->OnScroll((float)delta / (float)WHEEL_DELTA);
            }
            break;

        case WM_KILLFOCUS:
            Get().Reset();
            break;
    }
}

void WinAPIInputDispatchMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WINAPIInputBridge::Get().HandleMessage(hWnd, uMsg, wParam, lParam);
}

// ============================================================================
//  WINAPIWindow
// ============================================================================
int WINAPIWindow::make(std::string name, int w, int h)
{
    szAppName = name;
    WIDTH  = w;
    HEIGHT = h;

    auto hInstance = GetModuleHandleW(NULL);

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

    hWnd = CreateWindow(szAppName.c_str(),
                        "Generic OpenGL Sample",
                        WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                        CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT,
                        NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return 1;

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

void WINAPIWindow::SetEventHandler(EventHandler* _) { eh = _; }

void WINAPIWindow::WHOnCreate(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    hDC = GetDC(hWnd);

    PIXELFORMATDESCRIPTOR pfd, *ppfd = &pfd;
    int pixelformat;

    ppfd->nSize        = sizeof(PIXELFORMATDESCRIPTOR);
    ppfd->nVersion     = 1;
    ppfd->dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    ppfd->dwLayerMask  = PFD_MAIN_PLANE;
    ppfd->iPixelType   = PFD_TYPE_COLORINDEX;
    ppfd->cColorBits   = 8;
    ppfd->cDepthBits   = 16;
    ppfd->cAccumBits   = 0;
    ppfd->cStencilBits = 0;

    if ((pixelformat = ChoosePixelFormat(hDC, ppfd)) == 0)
        return;

    if (SetPixelFormat(hDC, pixelformat, ppfd) == FALSE)
        return;

    if (eh)
        eh->OnCreate(this);
}

void WINAPIWindow::WHOnPaint(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    PAINTSTRUCT ps;
    BeginPaint(hWnd, &ps);
    if (eh)
        eh->OnPaint(this);
    EndPaint(hWnd, &ps);
}

void WINAPIWindow::WHOnResize(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    WIDTH  = rect.right;
    HEIGHT = rect.bottom;
    if (eh)
        eh->OnResize(this, rect.top, rect.left, rect.bottom, rect.right);
}

void WINAPIWindow::WHOnDestroy(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    // заготовка
}

void WINAPIWindow::SetExtSize(size_t _) { ext.resize(_); }
size_t WINAPIWindow::GetExtSize()       { return ext.size(); }
void*  WINAPIWindow::GetExtImp()        { return ext.data(); }

void WINAPIWindow::Activate()
{
    WL_START_TYMETRACE;
    if (eh)
        eh->OnActivate(this);
}

void WINAPIWindow::Swap()
{
    WL_START_TYMETRACE;
    if (hDC)
        SwapBuffers(hDC);
    else if (eh)
        eh->OnSwap(this);
}

void WINAPIWindow::GetSize(int& w, int& h) const { w = WIDTH; h = HEIGHT; }

bool WINAPIWindow::Valid() const
{
    return hDC && WIDTH != 0 && HEIGHT != 0;
}

// ============================================================================
//  WINAPIWindowHelper
// ============================================================================
WINAPIWindowHelper& WINAPIWindowHelper::Get()
{
    static WINAPIWindowHelper _;
    return _;
}

int WINAPIWindowHelper::ProcessBegin()
{
    WL_START_TYMETRACE;
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

int WINAPIWindowHelper::ProcessEnd()
{
    WL_START_TYMETRACE;
    std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
    auto& vec = winlist();
    for (auto i = vec.begin(); i != vec.end(); )
    {
        auto el = *i;
        if (!el || (el->hDC && el->WIDTH == 0 && el->HEIGHT == 0))
        {
            i = vec.erase(i);
            continue;
        }
        if (el->hDC)
            SwapBuffers(el->hDC);
        ++i;
    }
    return 0;
}

Ref<IWindow> WINAPIWindowHelper::MakeWindow(IWindow::EventHandler* eh, std::string name, int WIDTH, int HEIGHT)
{
    auto& map = winmap();
    auto& vec = winlist();

    vec.push_back(MakeRef<WINAPIWindow>());
    auto back = vec.back();
    back->SetEventHandler(eh);
    back->make(name, WIDTH, HEIGHT);
    map[back->hWnd] = back.get();

    return back;
}

IWindow** WINAPIWindowHelper::GetWindows()
{
    std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
    auto& temp = winlist();

    cashed.clear();
    cashed.reserve(temp.size());
    for (auto& el : temp)
        cashed.push_back(el.get());

    return cashed.data();
}

int WINAPIWindowHelper::GetWindowsCount()
{
    std::lock_guard<std::mutex> lg(WINAPIWindowHelper::Get().winmutex);
    return (int)winlist().size();
}

// ============================================================================
//  WindowHandler
// ============================================================================
static LONG WINAPI WindowHandler(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WINAPIWindowHelper::Get().winmutex.lock();
    auto& map = winmap();
    auto& vec = winlist();
    WINAPIWindow* owner = vec.size() > 0
                          ? (vec.back()->hWnd == nullptr ? vec.back().get() : nullptr)
                          : nullptr;
    if (map.count(hWnd) != 0)
        owner = map.at(hWnd);
    WINAPIWindowHelper::Get().winmutex.unlock();

    if (!owner)
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    // Проброс в систему ввода
    WinAPIInputDispatchMessage(hWnd, uMsg, wParam, lParam);

    LONG lRet = 1;

    switch (uMsg)
    {
        case WM_CREATE:
            if (owner->hWnd != hWnd)
                owner->hWnd = hWnd;
            owner->WHOnCreate(wParam, lParam);
            break;

        case WM_PAINT:
            owner->WHOnPaint(wParam, lParam);
            break;

        case WM_SIZE:
            owner->WHOnResize(wParam, lParam);
            break;

        case WM_CLOSE:
            if (owner->hDC)
                ReleaseDC(hWnd, owner->hDC);
            owner->hDC    = 0;
            owner->WIDTH  = 0;
            owner->HEIGHT = 0;
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

// ============================================================================
//  Фабрики наружу
// ============================================================================
IWindowHelper* GetIWindowsHelper() { return &WINAPIWindowHelper::Get(); }
IInputHelper*  GetIInputHelper()   { return &WINAPIInputHelper::Get(); }
IInputBridge*  GetIInputBridge()   { return &WINAPIInputBridge::Get(); }

#endif // WLOG_OS_WINDOWS