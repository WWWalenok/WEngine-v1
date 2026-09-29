#ifndef __IINPUTHELPER_H__
#define __IINPUTHELPER_H__

#include "../core/core.h"

class IInputHelper;
IInputHelper* GetIInputHelper();

// ---------- Общие enum'ы (платформо-независимы) ----------

enum class KeyCode : uint16_t
{
    Unknown = 0,
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Num0, Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,
    Escape, Enter, Space, Tab, Backspace,
    LeftShift, RightShift, LeftCtrl, RightCtrl, LeftAlt, RightAlt,
    Up, Down, Left, Right,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    Count
};

enum class MouseButton : uint8_t
{
    Left = 0, Right, Middle, X1, X2, Count
};

enum class GamepadButton : uint8_t
{
    A, B, X, Y, LB, RB, LT, RT, Start, Back,
    DPadUp, DPadDown, DPadLeft, DPadRight, Count
};

enum class GamepadAxis : uint8_t
{
    LeftX, LeftY, RightX, RightY, TriggerLeft, TriggerRight, Count
};

// ---------- Базовый ресурс ввода (по образцу IRHIResource) ----------

class IInputDevice
{
public:
    virtual size_t GetType() const = 0;
    virtual ~IInputDevice() = default;
    static size_t StaticGetType() { return 0; }

    virtual void Update() = 0;
    virtual void ResetFrameState() = 0;
};

template<typename T, typename check = std::enable_if<std::is_base_of<IInputDevice, T>::value>::type>
static Ref<T> InputCast(Ref<IInputDevice> ref)
{
    if (ref && ref->GetType() == T::StaticGetType())
        return StaticRefCast<T>(ref);
    return nullptr;
}

#define INPUT_TYPE_IMP(Device)              \
    virtual size_t GetType() const override \
    { return GID(Device); }                 \
    static size_t StaticGetType()           \
    { return GID(Device); }

// ---------- Интерфейсы устройств ----------

class IKeyboard : public IInputDevice
{
public:
    INPUT_TYPE_IMP(IKeyboard)

    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsKeyUp(KeyCode key) const = 0;
    virtual bool IsKeyPressed(KeyCode key) const = 0;  // нажата в этом кадре
    virtual bool IsKeyReleased(KeyCode key) const = 0; // отпущена в этом кадре
};

class IMouse : public IInputDevice
{
public:
    INPUT_TYPE_IMP(IMouse)

    virtual bool IsButtonDown(MouseButton button) const = 0;
    virtual bool IsButtonPressed(MouseButton button) const = 0;
    virtual bool IsButtonReleased(MouseButton button) const = 0;

    virtual void GetPosition(int& x, int& y) const = 0;
    virtual void GetDelta(int& dx, int& dy) const = 0;
    virtual float GetScroll() const = 0;

    virtual void SetCursorVisible(bool visible) = 0;
    virtual bool IsCursorVisible() const = 0;
    virtual void SetCursorLocked(bool locked) = 0;
    virtual bool IsCursorLocked() const = 0;
};

class IGamepad : public IInputDevice
{
public:
    INPUT_TYPE_IMP(IGamepad)

    virtual int  GetSlot() const = 0;               // 0..N-1
    virtual bool IsConnected() const = 0;
    virtual bool IsButtonDown(GamepadButton button) const = 0;
    virtual bool IsButtonPressed(GamepadButton button) const = 0;
    virtual bool IsButtonReleased(GamepadButton button) const = 0;
    virtual float GetAxis(GamepadAxis axis) const = 0;
};

// ---------- Точка доступа (аналог IRHIHelper) ----------
//
// Никаких фабрик: набор устройств фиксирован.
// Реализация создаёт их в Init() и отдаёт через Get*.

constexpr int MAX_GAMEPADS = 4;

class IInputHelper
{
public:
    static IInputHelper* Get() { return GetIInputHelper(); }

    virtual Ref<IKeyboard> GetKeyboard() = 0;
    virtual Ref<IMouse>    GetMouse()    = 0;

    // slot в [0, MAX_GAMEPADS); для незанятых слотов может вернуть устройство
    // с IsConnected() == false, либо nullptr — на усмотрение реализации.
    virtual Ref<IGamepad>  GetGamepad(int slot) = 0;
    virtual int            GetGamepadCount()    = 0;

    virtual bool Init()     = 0;
    virtual bool Inited()   = 0;
    virtual void Shutdown() = 0;
};

#endif // __IINPUTHELPER_H__