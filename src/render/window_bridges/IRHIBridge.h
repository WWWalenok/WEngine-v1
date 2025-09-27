#pragma once

#include "../rhi_helpers/IRHIHelper.h"
#include "../window_helper/IWindowHelper.h"
#include <unordered_map>

struct IRHIBridge : public IWindow::EventHandler
{
    virtual int Render(ThredaPool*, WWorld*, UI*) = 0;
    virtual int Render(ThredaPool*, std::unordered_map<IWindow*, WWorld*>, std::unordered_map<IWindow*, UI*>) = 0;
};

static IRHIBridge* GetIRHIBridge();
