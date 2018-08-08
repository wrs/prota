/*
    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#include "gc.h"
#include "objects.h"
#include "interpreter.h"
#include "predefined.h"
#include <stdio.h>

inline void DebugBreak(void) { __asm__("int $3"); }

DECLARE_PSYM(array);
DECLARE_PSYM(real);

void TestFrames()
{
    Value f;
    Value f2;
    Value a;

    f = NewFrame();
    SetSlot(f, SYM(a), INT_V(1));
    SetSlot(f, SYM(f), f);

    a = NewArray(2);
    SetSlot(a, 0, NewString(_T("howdy")));
    SetSlot(a, 1, f);

    f2 = NewFrame();
    for (int i = 0; i < 70; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);

        Value v;
        switch (i % 3) {
            case 0: v = INT_V(i); break;
            case 1: v = a; break;
            case 2: v = NewArray(1); break;
        }

        SetSlot(f2, Intern(buf), v);
    }
    SetSlot(f, SYM(f2), f2);

    SetMaxPrintDepth(4);
    PrintValueLn(f);

    f2 = DeepClone(f);
    PrintValueLn(f2);

    f = NewFrame();
    SetSlot(f, SYM(x), INT_V(1));
    SetSlot(f, SYM(y), CHAR_V('a'));
    a = NewArray(PSYM(array), 0);
    AddArraySlot(a, INT_V(123));
    AddArraySlot(a, REAL_V(2.5));

    PrintValueLn(f);

    RemoveSlot(f, SYM(y));

    PrintValueLn(f);

    RemoveSlot(f, SYM(y));

    PrintValueLn(f);

    RemoveSlot(f, SYM(x));

    PrintValueLn(f);

    {
        f = NewFrame();
            SetSlot(f, SYM(int), INT_V(1));
            if (!HasSlot(f, SYM(int)))
            DebugBreak();
        Value v = GetSlot(f, SYM(int));
        if (V_INT(v) != 1)
            DebugBreak();
            SetSlot(f, SYM(char),CHAR_V('a'));
            if (!HasSlot(f, SYM(char)))
            DebugBreak();
        v = GetSlot(f, SYM(char));
        if (V_CHAR(v) != 'a')
            DebugBreak();
            PrintValueLn(f);

        // Remove the slots
        RemoveSlot(f, SYM(int));
            if (HasSlot(f, SYM(int)))
            DebugBreak();
            if (!HasSlot(f, SYM(char)))
            DebugBreak();
        RemoveSlot(f, SYM(char));
            if (HasSlot(f, SYM(char)))
            DebugBreak();
    }

    f = NewFrame();

    Value sym385 = Intern("SYM385");

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        SetSlot(f, Intern(buf), INT_V(i));
        ASSERT(GetObjLength(f) == i+1);
        if (i >= 385 && GetSlot(f, sym385) != INT_V(385))
            DebugBreak();
    }

    PrintValueLn(f);

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        ASSERT(V_INT(GetSlot(f, Intern(buf))) == i);
    }

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        SetSlot(f, Intern(buf), INT_V(500 - i));
        ASSERT(GetObjLength(f) == 500);
    }

    PrintValueLn(f);

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        ASSERT(V_INT(GetSlot(f, Intern(buf))) == 500 - i);
    }

    f2 = Clone(f);

    PrintValueLn(f2);

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        ASSERT(V_INT(GetSlot(f2, Intern(buf))) == 500 - i);
    }

    SetSlot(f2, SYM(postclone), INT_V(1234));
    ASSERT(GetSlot(f2, SYM(postclone)) == INT_V(1234));
    ASSERT(GetSlot(f, SYM(postclone)) == V_NIL);

    for (int i = 100; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        RemoveSlot(f, Intern(buf));
        ASSERT(HasSlot(f, SYM(SYM7)));
        ASSERT(HasSlot(f, SYM(SYM42)));
    }

    PrintValueLn(f);

    for (int i = 0; i < 500; i++) {
        char buf[32];
        sprintf(buf, "sym%d", i);
        if (i < 100)
            ASSERT(V_INT(GetSlot(f, Intern(buf))) == 500 - i);
        else
            ASSERT(!HasSlot(f, Intern(buf)));
    }

    f = V_NIL;
}


void teststr()
{
    PrintValueLn(ReadStreamFile("boot.stm"));
}

//__declspec(dllimport) void testiter(void);

void testintrp()
{
    Value func = ReadStreamFile("play.stm");
    PrintValueLn(func);
    PrintValueLn(Call(func));
}

int main()
{
    extern void PrintBCCounts(void);

    try {
        InitProtaLib();

        //TestFrames();
        //testiter();
        testintrp();
        //PrintBCCounts();

        EXPORT void TestParser();
        //TestParser();
    }
    catch (ProtaException& ex) {
        printf("exception: %s - ", ex.name);
        PrintValueLn(ex.data);
    }
    catch (const char* s) {
        printf("exception: %s\n", s);
    }

    GC_gcollect();

    return 0;
}
