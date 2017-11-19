/*
    Proto language runtime

    Bytecode definitions

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __OPCODES_H__
#define __OPCODES_H__

#define INSTR(a, b) (((a) << 3) | (b))

// A-field

enum {
    OP_UNARY0,
    OP_UNARY1,
    OP_UNARY2,
    OP_PUSH,
    OP_PUSHCONSTANT,
    OP_CALL,
    OP_INVOKE,
    OP_SEND,
    OP_SENDIFDEFINED,
    OP_RESEND,
    OP_RESENDIFDEFINED,
    OP_BRANCH,
    OP_BRANCHIFTRUE,
    OP_BRANCHIFFALSE,
    OP_FINDVAR,
    OP_GETVAR,
    OP_MAKEFRAME,
    OP_MAKEARRAY,
    OP_GETPATH,
    OP_SETPATH,
    OP_SETVAR,
    OP_FINDANDSETVAR,
    OP_INCRVAR,
    OP_BRANCHIFLOOPNOTDONE,
    OP_FREQFUNC,
    OP_NEWHANDLERS,
    OP_UNUSED26,
    OP_UNUSED27,
    OP_UNUSED28,
    OP_UNUSED29,
    OP_UNUSED30,
    OP_UNUSED31
};

// A-field = 0

enum {
    OP_POP,
    OP_DUP,
    OP_RETURN,
    OP_PUSHSELF,
    OP_SETLEXSCOPE,
    OP_ITERNEXT,
    OP_ITERDONE,
    OP_POPHANDLERS
};

// OP_FREQFUNC functions

enum {
    FF_ADD,
    FF_SUBTRACT,
    FF_AREF,
    FF_SETAREF,
    FF_EQUALS,
    FF_NOT,
    FF_NOTEQUALS,
    FF_MULTIPLY,
    FF_DIVIDE,
    FF_DIV,
    FF_LESSTHAN,
    FF_GREATERTHAN,
    FF_GREATEROREQUAL,
    FF_LESSOREQUAL,
    FF_BITAND,
    FF_BITOR,
    FF_BITNOT,
    FF_NEWITERATOR,
    FF_LENGTH,
    FF_CLONE,
    FF_SETCLASS,
    FF_ADDARRAYSLOT,
    FF_STRINGER,
    FF_HASPATH,
    FF_CLASSOF
};

#endif //__OPCODES_H__
