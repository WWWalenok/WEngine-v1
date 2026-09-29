#ifndef __INPUTSYSTEM_H__
#define __INPUTSYSTEM_H__

#include "../core/CoreSystem.h"
#include "IInputHelper.h"
#include "IInputBridge.h"

class InputSystem : public ISystem {
    DECLARE_SYSTEM(InputSystem);
    InputSystem() {
        helper = GetIInputHelper();
        bridge = GetIInputBridge();
    }

public:
    IInputHelper* helper;
    IInputBridge* bridge;

    // ---- Прямые Get-еры к устройствам ----
    Ref<IKeyboard> GetKeyboard()             { return helper->GetKeyboard(); }
    Ref<IMouse>    GetMouse()                { return helper->GetMouse(); }
    Ref<IGamepad>  GetGamepad(int slot)      { return helper->GetGamepad(slot); }
    int            GetGamepadCount()         { return helper->GetGamepadCount(); }

    // ---- Жизненный цикл ----
    void Start() {
        if (!helper || !bridge) return;
        helper->Init();

        // Прогреваем шину событий (как RenderSystem делает с bridge->Prepare/Finish)
        for (int i = 0; i < 10; ++i) {
            bridge->Prepare();
            bridge->Finish();
        }
    }

    void Shutdown() {
        if (helper) helper->Shutdown();
    }

    int Process() {
        if (!helper || !bridge) return 1;
        WL_START_TYMETRACE;

        bridge->Prepare();

        if (auto kb = helper->GetKeyboard()) kb->Update();
        if (auto ms = helper->GetMouse())    ms->Update();
        for (int i = 0; i < helper->GetGamepadCount(); ++i)
            if (auto gp = helper->GetGamepad(i)) gp->Update();

        bridge->Finish();
        return 0;
    }
};

#endif // __INPUTSYSTEM_H__