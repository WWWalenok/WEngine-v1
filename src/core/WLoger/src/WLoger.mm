#ifdef __OBJC__

#import <Foundation/Foundation.h>

#include "WLoger.cpp"

// https://github.com/a7ul/react-native-exception-handler/blob/master/ios/ReactNativeExceptionHandler.m

void __wloger_SignalHandler(int s)
{
    __wloger_generate_loger_buffer(WL_FATAL, true, "true", "SIGNAL_HANDLER", "", 0) << "Unhandled exception: " << s;
    __WLogerShutdown();
    signal(s, SIG_DFL);
    raise(s);
}

void __wloger_INIT_NATIVE() 
{
    signal(SIGABRT, __wloger_SignalHandler);
    signal(SIGILL, __wloger_SignalHandler);
    signal(SIGSEGV, __wloger_SignalHandler);
    signal(SIGFPE, __wloger_SignalHandler);
    signal(SIGBUS, __wloger_SignalHandler );
    //signal(SIGPIPE, SignalHandler);
    //Removing SIGPIPE as per https://github.com/master-atul/react-native-exception-handler/issues/32
    PWLI("REGISTERED RN EXCEPTION HANDLER");
}
#endif