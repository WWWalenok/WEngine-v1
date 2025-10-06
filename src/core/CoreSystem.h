#pragma once

#include "core.h"

class CoreSystem : public ISystem
{
    DECLARE_SYSTEM(CoreSystem);
    CoreSystem() :
        thread(0, true)
    {

    }
public:

    ThredaPool thread;

    int Process()
    {
        thread.Process();
        return 0;
    }
};