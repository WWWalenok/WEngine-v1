#ifndef __IINPUTBRIDGE_H__
#define __IINPUTBRIDGE_H__

#include "IInputHelper.h"

struct IInputBridge
{
    virtual ~IInputBridge() = default;

    virtual int  Prepare() = 0;
    virtual int  Finish()  = 0;
    virtual void Reset()   = 0;
};

IInputBridge* GetIInputBridge();

#endif // __IINPUTBRIDGE_H__