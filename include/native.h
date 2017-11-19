/*
    Copyright 1997-2000 Walter R. Smith

    HISTORY
    04/09/97    wrs     Started
*/

#ifndef __NATIVE_H__
#define __NATIVE_H__

#include "objects.h"

typedef Value (*NativeFuncPtr)(Value, Value*, void*);

// _x_ is for internal use only
#define DECLARE_NATIVE_FUNC(name)   Value name(Value rcvr, Value* _args_, void* _x_)

#define NATIVE_FUNC(name)   Value name(Value _rcvr_, Value* _args_, void* _x_)

#define ARGN(n) (_args_[(n)])
#define SELF    (_rcvr_)

#define NATIVE_ARGS_1(arg1) \
    struct _argstruct_ { Value arg1; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_2(arg1, arg2) \
    struct _argstruct_ { Value arg1; Value arg2; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_3(arg1, arg2, arg3) \
    struct _argstruct_ { Value arg1; Value arg2; Value arg3; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_4(arg1, arg2, arg3, arg4) \
    struct _argstruct_ { Value arg1; Value arg2; Value arg3; Value arg4; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_5(arg1, arg2, arg3, arg4, arg5) \
    struct _argstruct_ { Value arg1; Value arg2; Value arg3; Value arg4; Value arg5; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_6(arg1, arg2, arg3, arg4, arg5, arg6) \
    struct _argstruct_ { Value arg1; Value arg2; Value arg3; Value arg4; Value arg5; Value arg6; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;
#define NATIVE_ARGS_7(arg1, arg2, arg3, arg4, arg5, arg6, arg7) \
    struct _argstruct_ { Value arg1; Value arg2; Value arg3; Value arg4; Value arg5; Value arg6; Value arg7; }; \
    _argstruct_* _pArgs_ = (_argstruct_*) _args_;

#define ARG(name)   (_pArgs_->name)

struct GlobalFuncDecl {
    GlobalFuncDecl(char* theName, NativeFuncPtr theFunc, int theNumArgs)
        : name(theName), func(theFunc), numArgs(theNumArgs)
        { pNext = g_head; g_head = this; }

    char*           name;
    NativeFuncPtr   func;
    int             numArgs;
    GlobalFuncDecl* pNext;

    static GlobalFuncDecl* g_head;
};

#define DECLARE_GLOBAL_FUNCTION(name, func, nArgs) \
    GlobalFuncDecl gfd_##func(name, func, nArgs)

#endif  // __NATIVE_H__
