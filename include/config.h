/*
    Proto language runtime

    Configuration header

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __CONFIG_H__
#define __CONFIG_H__

typedef int Int32;
typedef unsigned int UInt32;

typedef unsigned int UInt;
typedef unsigned char Byte;

#include <assert.h>
#define ASSERT(exp) assert(exp)

#ifdef _DEBUG
#define NODEFAULT default: ASSERT(0); break;
#elif _MSC_VER >= 1200
#define NODEFAULT default: ASSERT(0); __assume(0); break;
#else
#define NODEFAULT default: break;
#endif

#ifdef _DEBUG
extern int dbgprintf(const char*, ...);
typedef struct _VALUE* Value;
extern void dbgPrintOneValue(Value v, int depth);
#define TRACE dbgprintf
#define TRACEVALUE dbgPrintOneValue
#else
inline void TRACE(char*, ...) { }
#define TRACEVALUE(Value, int) { }
#endif

#define ARRAYSIZE(x) (sizeof(x) / sizeof(x[0]))

#ifdef _MSC_VER
 #ifdef NSRT_BUILD
  #define EXPORT __declspec(dllexport) extern
 #else
  #define EXPORT __declspec(dllimport) extern
 #endif
#else
  #define EXPORT extern
#endif

#ifdef WIN32
 #include <malloc.h>
 #define ALLOCA _alloca
 #include <stdlib.h>
 #define SWAB _swab
 #include <tchar.h>
#endif

#endif //__CONFIG_H__
