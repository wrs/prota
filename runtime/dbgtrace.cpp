/*
    Proto language runtime

    Debugging and tracing functions

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include <stdarg.h>
#include <stdio.h>
#ifdef WIN32
 #include <windows.h>
#endif

int dbgprintf(const char* format, ...)
{
    va_list argptr;
    va_start(argptr, format);
#ifdef WIN32
    char buf[1024];
    int result = _vsnprintf(buf, sizeof(buf), format, argptr);
    OutputDebugStringA(buf);
#else
    int result = vfprintf(stderr, format, argptr);
#endif
    va_end(argptr);
    return result;
}
