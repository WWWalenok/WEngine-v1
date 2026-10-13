#ifndef __INPUTSYSTEM_H__
#define __INPUTSYSTEM_H__

#include "../core/core.h"
#include "../core/CoreSystem.h"
#include "IInputHelper.h"
#include "IInputBridge.h"

#ifndef __IINPUTEVENTS_H__
#define __IINPUTEVENTS_H__

#include "IInputHelper.h" // KeyCode, MouseButton, GamepadButton, GamepadAxis

struct InputEvents
{
    // ---- Клавиатура ----
    Slot OnAnyKeyPressed; // (KeyCode)  — однократное нажатие (переход up -> down)
    Slot OnAnyKeyReleased; // (KeyCode)  — однократное отпускание (down -> up)

    Slot OnKeyPressedList[(uint32_t)KeyCode::Count]; //   — однократное нажатие (переход up -> down)
    Slot OnKeyReleasedList[(uint32_t)KeyCode::Count]; //  — однократное отпускание (down -> up)

    inline Slot& OnKeyPressed(const KeyCode& key) { return OnKeyPressedList[(uint16_t)(key)]; } //   — однократное нажатие (переход up -> down)
    inline Slot& OnKeyReleased(const KeyCode& key) { return OnKeyReleasedList[(uint16_t)(key)]; } //  — однократное отпускание (down -> up)
    inline const Slot& OnKeyPressed(const KeyCode& key) const { return OnKeyPressedList[(uint16_t)(key)]; } //   — однократное нажатие (переход up -> down)
    inline const Slot& OnKeyReleased(const KeyCode& key) const { return OnKeyReleasedList[(uint16_t)(key)]; } //  — однократное отпускание (down -> up)

    Slot OnKeyDownList[(uint32_t)KeyCode::Count]; // — (float dt) — нажатие и удерживание (down)
    inline Slot& OnKeyDown(const KeyCode& key) { return OnKeyDownList[(uint16_t)(key)]; } // (float dt) — нажатие и удерживание (down)
    inline const Slot& OnKeyDown(const KeyCode& key) const { return OnKeyDownList[(uint16_t)(key)]; } // (float dt) — нажатие и удерживание  (down)

    // ---- Мышь ----
    Slot OnMouseButtonPressed; // (MouseButton)
    Slot OnMouseButtonReleased; // (MouseButton)
    Slot OnMouseMoved; // (int x, int y)  — новая абсолютная позиция
    Slot OnMouseDelta; // (int dx, int dy) — агрегированная дельта за кадр
    Slot OnMouseWheel; // (float delta)   — агрегированное колесо за кадр

    // ---- Геймпад (по аналогии; можно расширить) ----
    Slot OnGamepadButtonPressed; // (int slot, GamepadButton)
    Slot OnGamepadButtonReleased; // (int slot, GamepadButton)
};

#endif // __IINPUTEVENTS_H__

class InputSystem : public ISystem
{
    DECLARE_SYSTEM(InputSystem);
    InputSystem()
    {
        helper = GetIInputHelper();
        bridge = GetIInputBridge();
    }

    IInputHelper* helper;
    IInputBridge* bridge;
    InputEvents events;

public:
    Ref<IKeyboard> GetKeyboard()
    {
        return helper->GetKeyboard();
    }
    Ref<IMouse> GetMouse()
    {
        return helper->GetMouse();
    }
    Ref<IGamepad> GetGamepad(int s)
    {
        return helper->GetGamepad(s);
    }
    int GetGamepadCount()
    {
        return helper->GetGamepadCount();
    }

    InputEvents& Events()
    {
        return events;
    }

    void Start()
    {
        if (!helper || !bridge)
            return;
        helper->Init();
        for (int i = 0; i < 10; ++i)
        {
            bridge->Prepare();
            bridge->Finish();
        }
    }

    void Shutdown()
    {
        if (helper)
            helper->Shutdown();
    }

    int Process()
    {
        if (!helper || !bridge)
            return 1;
        WL_START_TYMETRACE;

        bridge->Prepare();

        if (auto kb = helper->GetKeyboard())
            kb->Update();
        if (auto ms = helper->GetMouse())
            ms->Update();
        for (int i = 0; i < helper->GetGamepadCount(); ++i)
            if (auto gp = helper->GetGamepad(i))
                gp->Update();

        DispatchEvents(); // <-- НОВОЕ

        bridge->Finish(); // сброс per-frame состояния
        return 0;
    }

private:
    using ts_t = decltype(std::chrono::high_resolution_clock::now().time_since_epoch());
    ts_t ts {ts_t(0)};
    void DispatchEvents()
    {
        WL_START_TYMETRACE;
        ts_t nts = std::chrono::high_resolution_clock::now().time_since_epoch();
        float dt = 0;
        if(ts.count() != 0)
        {
            dt = std::chrono::duration_cast<std::chrono::duration<float, std::ratio<1, 1>>> (nts - ts).count();
        }
        ts = nts;
        auto& ev = events;

        // ---- Клавиатура ----
        if (auto kb = helper->GetKeyboard())
        {
            for (size_t i = 0; i < (size_t)KeyCode::Count; ++i)
            {
                KeyCode k = (KeyCode)i;
                if (kb->IsKeyPressed(k))
                {
                    ev.OnAnyKeyPressed.Emit(k);
                    ev.OnKeyPressedList[i].Emit();
                }
                if (kb->IsKeyDown(k))
                {
                    ev.OnKeyDownList[i].Emit(dt);
                }
                if (kb->IsKeyReleased(k))
                {
                    ev.OnAnyKeyReleased.Emit(k);
                    ev.OnKeyPressedList[i].Emit();
                }
            }
        }

        // ---- Мышь ----
        if (auto ms = helper->GetMouse())
        {
            for (size_t i = 0; i < (size_t)MouseButton::Count; ++i)
            {
                MouseButton b = (MouseButton)i;
                if (ms->IsButtonPressed(b))
                    ev.OnMouseButtonPressed.Emit(b);
                if (ms->IsButtonReleased(b))
                    ev.OnMouseButtonReleased.Emit(b);
            }

            int dx = 0, dy = 0;
            ms->GetDelta(dx, dy);
            if (dx != 0 || dy != 0)
            {
                int x = 0, y = 0;
                ms->GetPosition(x, y);
                ev.OnMouseMoved.Emit(x, y);
                ev.OnMouseDelta.Emit(dx, dy);
            }

            float wheel = ms->GetScroll();
            if (wheel != 0.0f)
                ev.OnMouseWheel.Emit(wheel);
        }

        // ---- Геймпады ----
        for (int slot = 0; slot < helper->GetGamepadCount(); ++slot)
        {
            auto gp = helper->GetGamepad(slot);
            if (!gp || !gp->IsConnected())
                continue;

            for (size_t i = 0; i < (size_t)GamepadButton::Count; ++i)
            {
                GamepadButton b = (GamepadButton)i;
                if (gp->IsButtonPressed(b))
                    ev.OnGamepadButtonPressed.Emit(slot, b);
                if (gp->IsButtonReleased(b))
                    ev.OnGamepadButtonReleased.Emit(slot, b);
            }
        }
    }
};
#endif // __INPUTSYSTEM_H__