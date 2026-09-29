#pragma once

#include "core.h"

class CoreSystem : public ISystem
{
    DECLARE_SYSTEM(CoreSystem);
    CoreSystem() :
        thread(1)
    {

    }
public:

    ThreadPool thread;

    int Process()
    {
        thread.Process();
        return 0;
    }
};