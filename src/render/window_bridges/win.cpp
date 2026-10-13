
#include "../window_bridges/winapi_opengl.h"
#include "../opengl/OpenglRHIHelper.h"
#include "../opengl/opengl.h"


IRHIBridge* GetIRHIBridge() {
    static WinapiOpenglBridge bridge;
    return &bridge;
}

IRHIHelper *GetIRHIHelper()
{
    static OpenglRHIHelper instance;
    return &instance;
}