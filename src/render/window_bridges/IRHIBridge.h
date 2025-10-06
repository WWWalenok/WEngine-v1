#pragma once

#include "../rhi_helpers/IRHIHelper.h"
#include "../window_helper/IWindowHelper.h"
#include <unordered_map>

struct IRHIBridge : public IWindow::EventHandler
{
    virtual int Prepare() = 0;
    virtual int Render(WWorld*, UI*) = 0;
    virtual int Render(std::unordered_map<IWindow*, WWorld*>, std::unordered_map<IWindow*, UI*>) = 0;
    virtual int Finish() = 0;
};

IRHIBridge* GetIRHIBridge();
