/*
    Proto language runtime

    Main interpreter loop

    Copyright 1997-2000 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include "objects-private.h"
#include "interpreter.h"
#include "opcodes.h"
#include "predefined.h"
#include "gc.h"
#include <string.h>
#include <stdio.h>
#include "native.h"

DECLARE_PSYM(_implementor);
DECLARE_PSYM(_nextargframe);
DECLARE_PSYM(_proto);
DECLARE_PSYM(_parent);

Value   g_functions;
Value   g_variables;

struct StackFrame;

void    PrintInstruction(Byte* ip, Value* literals);
void    PrintVStack(Value* vsp, Value* vsTop, int items, int depth);
void    PrintCStack(StackFrame* csp, StackFrame* csTop, int items);

typedef Value* (*FreqFuncPtr)(Value*);

struct Function {
    Value   cls;
    Value   instrs;
    Value   literals;
    Value   argFrame;
    Value   numArgs;
};

struct NativeFunc {
    Value   cls;
    Value   fnPtr;
    Value   numArgs;
};

struct ArgFrame {
    Value   next;
    Value   rcvr;
    Value   impl;
};

Value   FindGlobalFunction(Value name)
{
    return GetSlot(g_functions, name);
}

void    SetGlobalFunction(Value name, Value func)
{
    SetSlot(g_functions, name, func);
}

bool    GetGlobalVar(Value name, Value* result)
{
    if (HasSlot(g_variables, name)) {
        *result = GetSlot(g_variables, name);
        return true;
    }
    else
        return false;
}

Value   GetGlobalVar(Value name)
{
    Value result;
    if (GetGlobalVar(name, &result))
        return result;
    else
        return V_NIL;
}

bool    SetGlobalVar(Value name, Value newValue, bool create)
{
    if (create || HasSlot(g_variables, name)) {
        SetSlot(g_variables, name, newValue);
        return true;
    }
    else
        return false;
}

bool    ProtoLookup(Value start, Value name, Value* where, Value* result)
{
    Value current = start;
    while (current != V_NIL) {
        if (HasSlot(current, name)) {
            *where = current;
            *result = GetSlot(current, name);
            return true;
        }
        current = GetSlot(current, PSYM(_proto));
    }
    return false;
}

bool    LexicalLookup(Value start, Value name, Value* result)
{
    Value current = start;
    while (current != V_NIL) {
        if (HasSlot(current, name)) {
            *result = GetSlot(current, name);
            return true;
        }
        current = ((ArgFrame*) V_PTR(current)->pSlots)->next;
    }
    return false;
}

bool    LexicalAssign(Value start, Value name, Value newValue)
{
    Value current = start;
    while (current != V_NIL) {
        if (HasSlot(current, name)) {
            SetSlot(current, name, newValue);
            return true;
        }
        current = ((ArgFrame*) V_PTR(current)->pSlots)->next;
    }
    return false;
}

bool    FullLookup(Value start, Value name, Value* where, Value* result)
{
    Value left = start;
    while (left != V_NIL) {
        if (ProtoLookup(left, name, where, result))
            return true;
        left = GetSlot(left, PSYM(_parent));
    }
    return false;
}

bool    Assign(Value start, Value name, Value newValue)
{
    Value left = start;
    while (left != V_NIL) {
        Value dummy;
        if (ProtoLookup(left, name, &dummy, &dummy)) {
            SetSlot(left, name, newValue);
            return true;
        }
        left = GetSlot(left, PSYM(_parent));
    }
    return false;
}

// Iterators ------------------------------------------------------

struct Iterator {
    Value   curTag;
    Value   curValue;
    Value   curObj;
    Value   deepNumSlots;
    Value   curSlot;
    Value   numSlots;
};

Value   NewIterator(Value obj, bool deeply)
{
    // BUGBUG: deeply not implemented
    ASSERT(!deeply);

    if (!V_ISPTR(obj))
        PROTO_THROW(g_exType, E_NotAPointer);
    Object* pObj = V_PTR(obj);
    int flags = pObj->flags;
    if (!(flags & HDR_SLOTTED))
        PROTO_THROW(g_exType, E_NotAPointer);

    Value iter = NewArray(SYM(iterator), sizeof(Iterator) / sizeof(Value));
    Iterator* pIter = (Iterator*) (V_PTR(iter)->pSlots);

    pIter->curObj = obj;
    int numSlots = pObj->size;
    pIter->numSlots = INT_V(numSlots);
    pIter->curSlot = INT_V(0);
    if (numSlots == 0)
        return iter;

    if (flags & HDR_FRAME) {
        pIter->curTag = GetMapTag(pObj->map, 0);
        pIter->curValue = pObj->pSlots[0];
    }
    else {
        pIter->curTag = INT_V(0);
        pIter->curValue = pObj->pSlots[0];
    }

    return iter;
}

bool    IteratorDone(Value iter)
{
    Iterator* pIter = (Iterator*) (V_PTR(iter)->pSlots);

    if (!V_ISPTR(pIter->curObj))
        PROTO_THROW(g_exType, E_NotAFrameOrArray);
    Object* pObj = V_PTR(pIter->curObj);
    int flags = pObj->flags;
    if (!(flags & HDR_SLOTTED))
        PROTO_THROW(g_exType, E_NotAFrameOrArray);

    pIter->numSlots = INT_V(pObj->size);

    if (pIter->curSlot >= pIter->numSlots)
        return true;
    else
        return false;
}

// BUGBUG: doesn't handle shrinking object

void    IteratorNext(Value iter)
{
    Iterator* pIter = (Iterator*) (V_PTR(iter)->pSlots);
    Object* pObj = V_PTR(pIter->curObj);
    int flags = pObj->flags;
    int index = V_INT(pIter->curSlot) + 1;
    pIter->curSlot = INT_V(index);

    if (flags & HDR_FRAME) {
        pIter->curTag = GetMapTag(pObj->map, index);
        pIter->curValue = pObj->pSlots[index];
    }
    else {
        pIter->curTag = INT_V(index);
        pIter->curValue = pObj->pSlots[index];
    }
}

EXPORT void testiter()
{
    Value a = NewArray(3);
    SetSlot(a, 0, INT_V(1));
    SetSlot(a, 1, INT_V(2));
    SetSlot(a, 2, INT_V(3));
    Value iter = NewIterator(a, false);
    while (!IteratorDone(iter)) {
        PrintValueLn(GetSlot(iter, 1));
        IteratorNext(iter);
    }
}

// ----------------------------------------------------------------
// The interpreter loop and associated stuff
// ----------------------------------------------------------------

struct Handler {
    Handler*    pNext;
    bool        used;
    Value       clauses;
    Value*      vsp;
    StackFrame* csp;
    ProtaException  ex;
};

struct StackFrame {
    Byte*   ip;
    Byte*   instrStart;
    Value*  locals;
    Value*  literals;
    Value   closure;
    Value   func;
    Value   rcvr;
    Value   impl;
    int     tempSize;
};

struct Process {
    Process(void* vsTop, void* csTop);
    void    Push(Value v);
    Value   Pop(void);
    Value   PeekN(int n);
    void    Drop(int n);
    void    Dup(void);
    void    PushFrame(void);
    void    PopFrame(void);
    void    SetupCall(Value fn, int actualNumArgs);
    bool    SetupSend(Value rcvr, Value start, Value name, int actualNumArgs, bool resend);
    void    Interpret(void);
    bool    HandleException(ProtaException& ex, StackFrame* cspLimit);

    Value*      m_vsp;
    StackFrame* m_csp;
    Value*      m_vsTop;
    StackFrame* m_csTop;
    Handler*    m_pHandler;
};

// Stacks are implemented as "full upward" --
// the stack pointer points to the cell containing the
// most recently pushed value, and the stack grows
// toward more positive addresses.

Process::Process(void* vsTop, void* csTop)
{
    m_vsTop = (Value*) vsTop;
    m_vsp = m_vsTop;
    m_csTop = (StackFrame*) csTop;
    // BUGBUG: Why not start m_csp at csTop-1?
    m_csp = m_csTop;
    memset(m_csp, 0, sizeof(StackFrame));
    m_pHandler = 0;
}

inline  void    Process::Push(Value v)
{
    *++m_vsp = v;
}

inline  Value   Process::Pop()
{
    Value v = *m_vsp;
    *m_vsp-- = 0;
    return v;
}

inline  Value   Process::PeekN(int n)
{
    return m_vsp[-n];
}

inline  void    Process::Drop(int n)
{
    m_vsp -= n;
    memset(m_vsp + 1, 0, n * sizeof(Value));
}

inline  void    Process::Dup()
{
    m_vsp[1] = m_vsp[0];
    m_vsp++;
}

inline  void    Process::PushFrame()
{
    ++m_csp;
}

inline  void    Process::PopFrame()
{
    // Clear all the Values in the stack frame so we don't have
    // any mysterious memory leaks.
    m_csp->closure = 0;
    m_csp->func = 0;
    m_csp->rcvr = 0;
    m_csp->impl = 0;

    m_csp--;
}

void    Process::SetupCall(Value fn, int actualNumArgs)
{
    Object* pObj = V_PTR(fn);
    if (!(pObj->flags & HDR_SLOTTED) || pObj->size < 1)
        PROTO_THROW(g_exType, E_NotAFunction);

    if (pObj->pSlots[0] == NATIVE_FN_CLASS) {
        NativeFunc* pFn = (NativeFunc*) pObj->pSlots;
        if (UNSAFE_V_INT(pFn->numArgs) != actualNumArgs)
            PROTO_THROW(g_exIntrp, E_WrongNumArgs);
        Value* pArgs = m_vsp - actualNumArgs + 1;
        Value result = ((NativeFuncPtr) pFn->fnPtr)(V_NIL, pArgs, this);
        Drop(actualNumArgs);
        Push(result);
    }
    else if (pObj->pSlots[0] == FUNCTION_CLASS) {
        Function* pFn = (Function*) pObj->pSlots;

        int numArgs = UNSAFE_V_INT(pFn->numArgs);
        int numLocals = ((unsigned int) numArgs) >> 16;
        numArgs &= 0xFFFF;

        if (numArgs != actualNumArgs)
            PROTO_THROW(g_exIntrp, E_WrongNumArgs);

        PushFrame();

        m_csp->locals = m_vsp - numArgs - 3 + 1;    // Pre-offset by the very historical 3
        m_csp->tempSize = numArgs + numLocals;
        for (int i = 0; i < numLocals; i++)
            Push(V_NIL);

        m_csp->func = fn;

        m_csp->ip = m_csp->instrStart = (Byte*) GetData(pFn->instrs);

        m_csp->literals = (pFn->literals == V_NIL) ? 0 : V_PTR(pFn->literals)->pSlots;

        if (pFn->argFrame == V_NIL) {
            m_csp->closure = V_NIL;
            m_csp->rcvr = V_NIL;
            m_csp->impl = V_NIL;
        }
        else {
            m_csp->closure = Clone(pFn->argFrame);
            ArgFrame* pAF = (ArgFrame*) V_PTR(pFn->argFrame)->pSlots;
            m_csp->rcvr = pAF->rcvr ? pAF->rcvr : V_NIL;
            m_csp->impl = pAF->impl ? pAF->rcvr : V_NIL;
        }
    }
    else
        PROTO_THROW(g_exType, E_NotAFunction);
}

bool    Process::SetupSend(Value rcvr, Value start, Value name, int actualNumArgs, bool resend)
{
    Value impl;
    Value fn;
    if (resend) {
        if (!ProtoLookup(start, name, &impl, &fn)) {
            // Not found--pop args and exit
            Drop(actualNumArgs);
            return false;
        }
    }
    else {
        if (!FullLookup(start, name, &impl, &fn)) {
            // Not found--pop args and exit
            Drop(actualNumArgs);
            return false;
        }
    }

    Object* pObj = V_PTR(fn);
    if (!(pObj->flags & HDR_SLOTTED) || pObj->size < 1)
        PROTO_THROW(g_exType, E_NotAFunction);

    if (pObj->pSlots[0] == NATIVE_FN_CLASS) {
        NativeFunc* pFn = (NativeFunc*) pObj->pSlots;
        if (UNSAFE_V_INT(pFn->numArgs) != actualNumArgs)
            PROTO_THROW(g_exIntrp, E_WrongNumArgs);
        Value* pArgs = m_vsp - actualNumArgs + 1;
        Value result = ((NativeFuncPtr) pFn->fnPtr)(rcvr, pArgs, this);
        Drop(actualNumArgs);
        Push(result);
    }
    else if (pObj->pSlots[0] == FUNCTION_CLASS) {
        Function* pFn = (Function*) pObj->pSlots;

        int numArgs = UNSAFE_V_INT(pFn->numArgs);
        int numLocals = ((unsigned int) numArgs) >> 16;
        numArgs &= 0xFFFF;

        if (numArgs != actualNumArgs)
            PROTO_THROW(g_exIntrp, E_WrongNumArgs);

        PushFrame();

        m_csp->locals = m_vsp - numArgs - 3 + 1;    // Pre-offset by the very historical 3
        m_csp->tempSize = numArgs + numLocals;
        for (int i = 0; i < numLocals; i++)
            Push(V_NIL);

        m_csp->func = fn;

        m_csp->ip = m_csp->instrStart = (Byte*) GetData(pFn->instrs);

        m_csp->literals = (pFn->literals == V_NIL) ? 0 : V_PTR(pFn->literals)->pSlots;

        m_csp->rcvr = rcvr;
        m_csp->impl = impl;

        if (pFn->argFrame == V_NIL)
            m_csp->closure = V_NIL;
        else {
            m_csp->closure = Clone(pFn->argFrame);
            ArgFrame* pAF = (ArgFrame*) V_PTR(pFn->argFrame)->pSlots;
            if (pAF->rcvr)
                pAF->rcvr = rcvr;
            if (pAF->impl)
                pAF->impl = impl;
        }
    }
    else
        PROTO_THROW(g_exType, E_NotAFunction);

    return true;
}

#define BCCOUNT

#ifdef BCCOUNT
int g_bcCounts[256];
int g_ffCounts[32];

EXPORT  void    PrintBCCounts();
#endif

void    Process::Interpret()
{
    int param;
    StackFrame* initialCSP = m_csp;
    StackFrame* csp = m_csp;

    while (1) {
        try {
            while (1) {
                PrintCStack(csp, m_csTop, 6);
                TRACE("\n");
                PrintVStack(m_vsp, m_vsTop, 6, 0);
                TRACE("\n\t%X@%d: ", (int) csp->func, csp->ip - csp->instrStart);
                PrintInstruction(csp->ip, csp->literals);
                TRACE("\n");

                unsigned char op = *csp->ip++;
                int Bfield = op & 7;    // workaround vc bug

        #ifdef BCCOUNT
                g_bcCounts[op]++;
        #endif

            #define EIGHTCASE_(op, sign)    \
                case INSTR(op, 7):  \
                    param = (((sign char *)csp->ip)[0] << 8) | csp->ip[1];  \
                    csp->ip += 2;   \
                    goto lbl_##op;  \
                case INSTR(op, 0): case INSTR(op, 1): case INSTR(op, 2): case INSTR(op, 3): \
                case INSTR(op, 4): case INSTR(op, 5): case INSTR(op, 6):    \
                    param = Bfield; \
                lbl_##op:   ;

            #define EIGHTCASE(op) EIGHTCASE_(op, unsigned)
            #define EIGHTCASE_SIGNED(op) EIGHTCASE_(op, signed)

                switch (op) {
                case OP_POP:
                    Pop();
                    break;

                case OP_DUP:
                    Dup();
                    break;

                case OP_RETURN:
                    {
                        // BUGBUG: technically zero or >1 results might be on the stack
                        if (csp->tempSize) {
                            Value result = Pop();
                            Drop(csp->tempSize);
                            Push(result);
                        }
                        if (csp == initialCSP)
                            return;
                        PopFrame();
                        csp = m_csp;
                        break;
                    }

                case OP_PUSHSELF:
                    Push(csp->rcvr);
                    break;

                case OP_SETLEXSCOPE:
                {
                    Value func = Clone(Pop());
                    Function* pFunc = (Function*) V_PTR(func)->pSlots;
                    Value argFrame = Clone(pFunc->argFrame);
                    ArgFrame* pAF = (ArgFrame*) V_PTR(argFrame)->pSlots;
                    pAF->next = csp->closure;
                    if (pAF->rcvr)
                        pAF->rcvr = csp->rcvr;
                    if (pAF->impl)
                        pAF->impl = csp->impl;
                    pFunc->argFrame = argFrame;
                    Push(func);
                    break;
                }

                case OP_ITERNEXT:
                    IteratorNext(Pop());
                    break;

                case OP_ITERDONE:
                    Push(BOOL_V(IteratorDone(Pop())));
                    break;

                case OP_POPHANDLERS:
                {
                    // Due to historical stupidity, the pophandlers instruction is the
                    // nonexistent eighth unary1op. Its encoding is 07 00 07! We assume
                    // there won't be any more 3-byte unary1ops and just assume all 07s
                    // are 07 00 07's.
                    csp->ip += 2;

                    ASSERT(m_pHandler != 0);
                    ASSERT(csp == m_pHandler->csp);
                    Handler* pDead = m_pHandler;
                    m_pHandler = pDead->pNext;
                    GC_FREE(pDead);
                    break;
                }

                EIGHTCASE(OP_PUSH)
                    Push(csp->literals[param]);
                    break;

                // B field is signed for OP_PUSHCONSTANT, so EIGHTCASE won't work
                EIGHTCASE_SIGNED(OP_PUSHCONSTANT)
                    Push((Value) param);
                    break;

                EIGHTCASE(OP_CALL)
                {
                    Value name = Pop();
                    Value func = FindGlobalFunction(name);
                    if (func == V_NIL)
                        PROTO_THROW(g_exIntrp, E_UndefinedFunction);
                    SetupCall(func, param);
                    csp = m_csp;
                    break;
                }

                EIGHTCASE(OP_INVOKE)
                {
                    Value func = Pop();
                    SetupCall(func, param);
                    csp = m_csp;
                    break;
                }

                EIGHTCASE(OP_SEND)
                {
                    Value name = Pop();
                    Value rcvr = Pop();
                    TRACEVALUE(rcvr, 2);
                    TRACE("\n");
                    if (!SetupSend(rcvr, rcvr, name, param, false))
                        PROTO_THROW(g_exIntrp, E_UndefinedMethod);
                    csp = m_csp;
                    break;
                }

                EIGHTCASE(OP_SENDIFDEFINED)
                {
                    Value name = Pop();
                    Value rcvr = Pop();
                    if (SetupSend(rcvr, rcvr, name, param, false))
                        csp = m_csp;
                    else
                        Push(V_NIL);
                    break;
                }

                EIGHTCASE(OP_RESEND)
                {
                    Value name = Pop();
                    if (!SetupSend(csp->rcvr, GetSlot(csp->impl, PSYM(_proto)), name, param, true))
                        PROTO_THROW(g_exIntrp, E_UndefinedMethod);
                    csp = m_csp;
                    break;
                }

                EIGHTCASE(OP_RESENDIFDEFINED)
                {
                    Value name = Pop();
                    if (SetupSend(csp->rcvr, GetSlot(csp->impl, PSYM(_proto)), name, param, true))
                        csp = m_csp;
                    else
                        Push(V_NIL);
                    break;
                }

                EIGHTCASE(OP_BRANCH)
                    csp->ip = csp->instrStart + param;
                    break;

                EIGHTCASE(OP_BRANCHIFTRUE)
                    if (Pop() != V_NIL)
                        csp->ip = csp->instrStart + param;
                    break;

                EIGHTCASE(OP_BRANCHIFFALSE)
                    if (Pop() == V_NIL)
                        csp->ip = csp->instrStart + param;
                    break;

                EIGHTCASE(OP_FINDVAR)
                {
                    Value name = csp->literals[param];
                    Value value;
                    Value dummy;
                    if (LexicalLookup(csp->closure, name, &value))
                        Push(value);
                    else if (FullLookup(csp->rcvr, name, &dummy, &value))
                        Push(value);
                    else if (GetGlobalVar(name, &value))
                        Push(value);
                    else
                        PROTO_THROW(g_exIntrp, E_UndefinedVariable);
                    break;
                }

                EIGHTCASE(OP_GETVAR)
                    Push(csp->locals[param]);
                    break;

                EIGHTCASE(OP_MAKEFRAME)
                {
                    Value frame = NewFrameWithMap(Pop());
                    Value* pSlots = V_PTR(frame)->pSlots;
                    for (int i = 0; i < param; i++)
                        pSlots[i] = PeekN(param - i - 1);
                    Drop(param);
                    Push(frame);
                    break;
                }

                EIGHTCASE(OP_MAKEARRAY)
                {
                    Value cls = Pop();
                    if (param == 0xFFFF)
                        Push(NewArray(cls, UNSAFE_V_INT(Pop())));
                    else {
                        Value array = NewArray(cls, param);
                        Value* pSlots = V_PTR(array)->pSlots;
                        for (int i = 0; i < param; i++)
                            pSlots[i] = PeekN(param - i - 1);
                        Drop(param);
                        Push(array);
                    }
                    break;
                }

                EIGHTCASE(OP_GETPATH)
                {
                    Value path = Pop();
                    Value obj = Pop();
                    if (obj == V_NIL) {
                        if (param == 0)
                            Push(V_NIL);
                        else
                            PROTO_THROW(g_exFr, E_PathFailed);
                    }
                    else
                        Push(GetPath(obj, path));
                    break;
                }

                EIGHTCASE(OP_SETPATH)
                {
                    Value newValue = Pop();
                    Value path = Pop();
                    Value obj = Pop();
                    SetPath(obj, path, newValue);
                    if (param == 1)
                        Push(newValue);
                    break;
                }

                EIGHTCASE(OP_SETVAR)
                {
                    csp->locals[param] = Pop();
                    break;
                }

                EIGHTCASE(OP_FINDANDSETVAR)
                {
                    Value name = csp->literals[param];
                    Value value = Pop();
                    if (!LexicalAssign(csp->closure, name, value) &&
                            !Assign(csp->rcvr, name, value) &&
                            !SetGlobalVar(name, value, false)) {
                        // Undefined local
                        if (csp->closure == V_NIL) {
                            // BUGBUG: should clone prototypical argframe?
                            csp->closure = NewFrame();
                            SetSlot(csp->closure, PSYM(_nextargframe), V_NIL);
                            SetSlot(csp->closure, PSYM(_parent), V_NIL);
                            SetSlot(csp->closure, PSYM(_implementor), V_NIL);
                        }
                        SetSlot(csp->closure, name, value);
                    }
                    break;
                }

                EIGHTCASE(OP_INCRVAR)
                {
                    int addend = V_INT(PeekN(0));
                    Value result = INT_V(addend + V_INT(csp->locals[param]));
                    csp->locals[param] = result;
                    Push(result);
                    break;
                }

                EIGHTCASE(OP_BRANCHIFLOOPNOTDONE)
                {
                    int limit = V_INT(Pop());
                    int index = V_INT(Pop());
                    int incr = V_INT(Pop());
                    if (incr == 0)
                        PROTO_THROW(g_exIntrp, E_ZeroForLoopIncr);
                    else if ((incr > 0 && index <= limit) || (incr < 0 && index >= limit))
                        csp->ip = csp->instrStart + param;
                    break;
                }

                EIGHTCASE(OP_FREQFUNC)
                {
        #ifdef BCCOUNT
                    g_ffCounts[param]++;
        #endif

                    switch (param) {
                    // BUGBUG: all numerics are broken (slow & integer only)
        #define BINOP(cvt, oper)    \
                    {   \
                        int b = V_INT(Pop());   \
                        int a = V_INT(Pop());   \
                        Push(cvt(a oper b));    \
                    }

                    case FF_ADD:
                        BINOP(INT_V, +)
                        break;

                    case FF_SUBTRACT:
                        BINOP(INT_V, -);
                        break;

                    case FF_MULTIPLY:
                        BINOP(INT_V, *);
                        break;

                    case FF_DIVIDE:
                    {
                        int b = V_INT(Pop());
                        int a = V_INT(Pop());
                        Push(REAL_V((double) a / b));
                        break;
                    }

                    case FF_DIV:
                        BINOP(INT_V, /);
                        break;

                    case FF_AREF:
                    {
                        int index = V_INT(Pop());
                        Value obj = Pop();
                        // BUGBUG: string access not implemented
                        Push(GetSlot(obj, index));
                        break;
                    }

                    case FF_SETAREF:
                    {
                        Value elt = Pop();
                        int index = V_INT(Pop());
                        Value obj = Pop();
                        // BUGBUG: string access not implemented
                        SetSlot(obj, index, elt);
                        Push(elt);
                        break;
                    }

                    case FF_NEWITERATOR:
                    {
                        Value deeply = Pop();
                        Value obj = Pop();
                        Push(NewIterator(obj, V_BOOL(deeply)));
                        break;
                    }

                    case FF_LENGTH:
                    {
                        Push(INT_V(GetObjLength(Pop())));
                        break;
                    }

                    case FF_ADDARRAYSLOT:
                    {
                        Value elt = Pop();
                        Value array = Pop();
                        AddArraySlot(array, elt);
                        Push(elt);
                        break;
                    }

                    case FF_EQUALS:
                        Push(BOOL_V(V_EQ(Pop(), Pop())));
                        break;

                    case FF_NOTEQUALS:
                        Push(BOOL_V(!V_EQ(Pop(), Pop())));
                        break;

                    // BUGBUG: all compares are broken (slow & integer only)

                    case FF_LESSTHAN:
                        BINOP(BOOL_V, <);
                        break;

                    case FF_GREATERTHAN:
                        BINOP(BOOL_V, >);
                        break;

                    case FF_LESSOREQUAL:
                        BINOP(BOOL_V, <=);
                        break;

                    case FF_GREATEROREQUAL:
                        BINOP(BOOL_V, >=);
                        break;

                    case FF_NOT:
                        Push(BOOL_V(!V_BOOL(Pop())));
                        break;

                    case FF_BITAND:
                        BINOP(INT_V, &);
                        break;

                    case FF_BITOR:
                        BINOP(INT_V, |);
                        break;

                    case FF_BITNOT:
                        Push(INT_V(~ V_INT(Pop())));
                        break;

                    case FF_SETCLASS:
                    {
                        Value cls = Pop();
                        Value obj = Pop();
                        SetClassSlot(obj, cls);
                        Push(obj);
                        break;
                    }

                    case FF_CLASSOF:
                        Push(GetClassSlot(Pop()));
                        break;

                    case FF_CLONE:
                        Push(Clone(Pop()));
                        break;

                    case FF_STRINGER:
                        // BUGBUG: not implemented
                        assert(0);
                        Push(V_NIL);
                        break;

                    case FF_HASPATH:
                    {
                        Value path = Pop();
                        Value obj = Pop();
                        Push(BOOL_V(HasPath(obj, path)));
                        break;
                    }

                    default:
                        assert(0);
                    }

                    break;
                }

                EIGHTCASE(OP_NEWHANDLERS)
                {
                    Handler* pNew = (Handler*) GC_MALLOC(sizeof(Handler));
                    pNew->pNext = m_pHandler;
                    pNew->clauses = NewArray(SYM(handlers), param * 2);
                    memcpy(GetArraySlots(pNew->clauses), m_vsp - param * 2 + 1, param * 2 * sizeof(Value));
                    Drop(param * 2);
                    pNew->vsp = m_vsp;
                    pNew->csp = m_csp;
                    m_pHandler = pNew;
                    break;
                }

                EIGHTCASE(OP_UNARY1)
                EIGHTCASE(OP_UNARY2)
                EIGHTCASE(OP_UNUSED26)
                EIGHTCASE(OP_UNUSED27)
                EIGHTCASE(OP_UNUSED28)
                EIGHTCASE(OP_UNUSED29)
                EIGHTCASE(OP_UNUSED30)
                EIGHTCASE(OP_UNUSED31)
                    PROTO_THROW(g_exIntrp, E_InvalidBytecode);
                    break;

                default:
                    assert(0);
                }
            }
        }
        catch (ProtaException& ex) {
            if (!HandleException(ex, initialCSP))
                throw;
            csp = m_csp;
        }
        catch (...) {
            ProtaException ex("evt.ex", V_NIL);
            if (!HandleException(ex, initialCSP))
                throw;
            csp = m_csp;
        }
    }
}

bool    Process::HandleException(ProtaException& ex, StackFrame* cspLimit)
{
    while (m_pHandler != 0 && m_pHandler->csp >= cspLimit) {
        if (!m_pHandler->used) {
            int numHandlers = GetArrayLength(m_pHandler->clauses) / 2;
            for (int i = 0; i < numHandlers; i++) {
                Value nameSym = GetSlot(m_pHandler->clauses, i * 2);
                if (Subexception(ex.name, SymbolName(nameSym))) {
                    m_pHandler->used = true;
                    m_pHandler->ex = ex;
                    m_vsp = m_pHandler->vsp;
                    m_csp = m_pHandler->csp;
                    m_csp->ip = m_csp->instrStart + V_INT(GetSlot(m_pHandler->clauses, i * 2 + 1));
                    return true;
                }
            }
        }

        m_pHandler = m_pHandler->pNext;
    }

    return false;
}

// Is name1 a subexception of name2?
// If name2 is the same as name1, or if name2 is a prefix of name1
// that is followed by a '.', then yes.
// BUGBUG: Should support semicolon and type.xxx

bool    Subexception(const char* name1, const char* name2)
{
    while (*name1 && *name2 && *name1 == *name2) {
        name1++;
        name2++;
    }
    if (*name1 == 0 && *name2 == 0)
        return true;
    else if (*name2 == 0 && *name1 == '.')
        return true;
    else
        return false;
}

NATIVE_FUNC(FThrow)
{
    NATIVE_ARGS_2(name, data);

    throw ProtaException(SymbolName(ARG(name)), ARG(data));

    return V_NIL;
}

DECLARE_GLOBAL_FUNCTION("Throw", FThrow, 2);

NATIVE_FUNC(FCurrentException)
{
    // Use secret third arg
    Process* pProcess = (Process*) _x_;

    Handler* pHandler = pProcess->m_pHandler;
    while (pHandler != 0 && !pHandler->used)
        pHandler = pHandler->pNext;

    if (pHandler == 0)
        return V_NIL;

    Value result = NewFrame();
    SetSlot(result, SYM(name), Intern(pHandler->ex.name));
    SetSlot(result, SYM(data), pHandler->ex.data);

    return result;
}

DECLARE_GLOBAL_FUNCTION("CurrentException", FCurrentException, 0);

// BUGBUG: Obviously a wacko placeholder

EXPORT  Value   Call(Value func)
{
    Value* stack = (Value*) GC_MALLOC(1000 * sizeof(Value));
    Process p(stack, stack + 500);
    p.SetupCall(func, 0);
    p.Interpret();
    return p.Pop();
}

Value   MakeNativeFunc(NativeFuncPtr fnPtr, int numArgs)
{
    Value f = NewFrame();
    SetSlot(f, SYM(class), NATIVE_FN_CLASS);
    SetSlot(f, SYM(entry), (Value) fnPtr);
    SetSlot(f, SYM(numArgs), INT_V(numArgs));
    return f;
}

NATIVE_FUNC(FBNot)
{
    NATIVE_ARGS_1(value);

    return INT_V(~ V_INT(ARG(value)));
}

DECLARE_GLOBAL_FUNCTION("BNot", FBNot, 1);

struct GlobalFuncDecl* GlobalFuncDecl::g_head;

EXPORT  void    InitInterpreter()
{
    g_functions = NewFrame();
    g_variables = NewFrame();
    SetSlot(g_variables, SYM(vars), g_variables);
    SetSlot(g_variables, SYM(functions), g_functions);

    for (GlobalFuncDecl* pGFD = GlobalFuncDecl::g_head; pGFD != 0; pGFD = pGFD->pNext)
        SetSlot(g_functions, Intern(pGFD->name), MakeNativeFunc(pGFD->func, pGFD->numArgs));
}

// DEBUGGING

static const char* g_opNames[] = {
    "unary0",
    "unary1",
    "unary2",
    "push",
    "push-constant",
    "call",
    "invoke",
    "send",
    "send-if-defined",
    "resend",
    "resend-if-defined",
    "branch",
    "branch-if-true",
    "branch-if-false",
    "find-var",
    "get-var",
    "make-frame",
    "make-array",
    "get-path",
    "set-path",
    "set-var",
    "find-and-set-var",
    "incr-var",
    "branch-if-loop-not-done",
    "freq-func",
    "new-handlers",
    "unused26",
    "unused27",
    "unused28",
    "unused29",
    "unused30",
    "unused31"
};

static const char* g_unary0Names[] = {
    "pop",
    "dup",
    "return",
    "push-self",
    "set-lex-scope",
    "iter-next",
    "iter-done",
    "pop-handlers"
};

static const char* g_freqFuncNames[] = {
    "+",
    "-",
    "ARef",
    "SetARef",
    "=",
    "not",
    "<>",
    "*",
    "/",
    "div",
    "<",
    ">",
    ">=",
    "<=",
    "BAnd",
    "BOr",
    "BNot",
    "NewIterator",
    "Length",
    "Clone",
    "SetClass",
    "AddArraySlot",
    "Stringer",
    "HasPath",
    "ClassOf"
};

void    PrintInstruction(Byte* ip, Value* literals)
{

    int op = *ip;
    int A = op >> 3;
    int B = op & 7;
    if (B == 7) {
        if (A == OP_PUSHCONSTANT)
            B = (((signed char *)ip)[1] << 8) | ip[2];
        else
            B = (((unsigned char *)ip)[1] << 8) | ip[2];
    }

    switch (A) {
    case OP_UNARY0:
        TRACE("%s", g_unary0Names[B]);
        break;

    case OP_PUSHCONSTANT:
        TRACE("%s <", g_opNames[A]);
        TRACEVALUE((Value) B, 0);
        TRACE(">");
        break;

    case OP_PUSH: case OP_FINDVAR: case OP_FINDANDSETVAR:
        TRACE("%s <", g_opNames[A]);
        TRACEVALUE(literals[B], 0);
        TRACE(">");
        break;

    case OP_FREQFUNC:
        if (B < ARRAYSIZE(g_freqFuncNames))
            TRACE("%s", g_freqFuncNames[B]);
        else
            TRACE("freq-func %d", B);
        break;

    default:
        TRACE("%s %d", g_opNames[A], B);
        break;
    }
}

// Print the most recent nItems values on the value stack
// in chronological order

void    PrintVStack(Value* vsp, Value* vsTop, int nItems, int depth)
{
    Value* limit = vsp - nItems + 1;
    if (limit < vsTop)
        limit = vsTop;

    for (Value* sp = limit; sp <= vsp; sp++) {
        TRACEVALUE(*sp, depth);
        TRACE(" ");
    }
}

// Print the most recent nItems frames on the control stack
// in chronological order

void    PrintCStack(StackFrame* csp, StackFrame* csTop, int nItems)
{
    StackFrame* limit = csp - nItems + 1;
    if (limit < csTop)
        limit = csTop;

    // There's an empty frame at the top of the control stack--no point
    // in printing it.

    for (StackFrame* sfp = limit + 1; sfp <= csp; sfp++)
        TRACE("%X@%d ", (int) sfp->func, sfp->ip - sfp->instrStart);
}

#ifdef BCCOUNT

EXPORT  void    PrintBCCounts()
{
    for (int uop = OP_POP; uop <= OP_POPHANDLERS; uop++)
        TRACE("%20s\t%d\n", g_unary0Names[uop], g_bcCounts[uop]);
    for (int op = OP_PUSH; op <= OP_NEWHANDLERS; op++) {
        TRACE("%20s", g_opNames[op]);
        for (int b = 0; b <= 7; b++)
            TRACE("\t%d", g_bcCounts[INSTR(op, b)]);
        TRACE("\n");
    }
    for (int ff = FF_ADD; ff <= FF_CLASSOF; ff++)
        TRACE("%20s\t%d\n", g_freqFuncNames[ff], g_ffCounts[ff]);
}

#endif
