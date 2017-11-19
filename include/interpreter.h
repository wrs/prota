/*
    Proto language runtime

    Interpreter interface

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __INTERPRETER_H__
#define __INTERPRETER_H__

#include "objects.h"

EXPORT  Value   Call(Value func);

EXPORT  Value   GetGlobalVar(Value name);
EXPORT  bool    GetGlobalVar(Value name, Value* result);
EXPORT  bool    SetGlobalVar(Value name, Value newValue, bool create = true);
EXPORT  Value   GetGlobalFunction(Value name);
EXPORT  void    SetGlobalFunction(Value name, Value func);

#endif //__INTERPRETER_H__
