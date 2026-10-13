#ifndef __IRHIBRIDGE_H__
#define __IRHIBRIDGE_H__

#include "../core/core.h"
#include "IRHIHelper.h"
#include "IWindowHelper.h"

class RenderGraphBuilder;

class IRHIBridge : public IWindow::EventHandler
{
public:
    virtual int Prepare() = 0;
    virtual int Finish() = 0;
};

extern IRHIBridge* GetIRHIBridge();

#endif // __IRHIBRIDGE_H__