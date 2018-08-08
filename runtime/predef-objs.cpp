/*
    Prota language runtime

    Compile-time objects

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

// Someday this will be automatically generated

#include "config.h"
#include "predefined.h"

typedef struct _VALUE* Value;

struct Binary {
    UInt    size : 28;
    UInt    flags : 4;
    Value   cls;
    void*   pData;
};

const Value     SYMBOL_CLASS    = (Value) 0x55552;
void    InternPredefSyms(Value syms[], int len);


// REMAINDER OF THIS FILE IS AUTOGENERATED

extern char data_append[];
extern char data_array[];
extern char data_bottom[];
extern char data_call[];
extern char data_class[];
extern char data_cobj[];
extern char data_data[];
extern char data_entry[];
extern char data_errcode[];
extern char data_functions[];
extern char data_getstring[];
extern char data_iterator[];
extern char data_left[];
extern char data_numargs[];
extern char data_real[];
extern char data_reset[];
extern char data_send[];
extern char data_string[];
extern char data_pathexpr[];
extern char data_printdepth[];
extern char data_right[];
extern char data_top[];
extern char data_vars[];
extern char data__implementor[];
extern char data__nextargframe[];
extern char data__proto[];
extern char data__parent[];
PREDEF(Binary, PREDEF_SYM_NAME(append)) = { 11, 0, SYMBOL_CLASS, &data_append };
PREDEF(Binary, PREDEF_SYM_NAME(array)) = { 10, 0, SYMBOL_CLASS, &data_array };
PREDEF(Binary, PREDEF_SYM_NAME(bottom)) = { 11, 0, SYMBOL_CLASS, &data_bottom };
PREDEF(Binary, PREDEF_SYM_NAME(call)) = { 9, 0, SYMBOL_CLASS, &data_call };
PREDEF(Binary, PREDEF_SYM_NAME(class)) = { 10, 0, SYMBOL_CLASS, &data_class };
PREDEF(Binary, PREDEF_SYM_NAME(cobj)) = { 9, 0, SYMBOL_CLASS, &data_cobj };
PREDEF(Binary, PREDEF_SYM_NAME(data)) = { 9, 0, SYMBOL_CLASS, &data_data };
PREDEF(Binary, PREDEF_SYM_NAME(entry)) = { 10, 0, SYMBOL_CLASS, &data_entry };
PREDEF(Binary, PREDEF_SYM_NAME(errcode)) = { 12, 0, SYMBOL_CLASS, &data_errcode };
PREDEF(Binary, PREDEF_SYM_NAME(functions)) = { 14, 0, SYMBOL_CLASS, &data_functions };
PREDEF(Binary, PREDEF_SYM_NAME(getstring)) = { 14, 0, SYMBOL_CLASS, &data_getstring };
PREDEF(Binary, PREDEF_SYM_NAME(iterator)) = { 13, 0, SYMBOL_CLASS, &data_iterator };
PREDEF(Binary, PREDEF_SYM_NAME(left)) = { 9, 0, SYMBOL_CLASS, &data_left };
PREDEF(Binary, PREDEF_SYM_NAME(numargs)) = { 12, 0, SYMBOL_CLASS, &data_numargs };
PREDEF(Binary, PREDEF_SYM_NAME(real)) = { 9, 0, SYMBOL_CLASS, &data_real };
PREDEF(Binary, PREDEF_SYM_NAME(reset)) = { 10, 0, SYMBOL_CLASS, &data_reset };
PREDEF(Binary, PREDEF_SYM_NAME(send)) = { 9, 0, SYMBOL_CLASS, &data_send };
PREDEF(Binary, PREDEF_SYM_NAME(string)) = { 11, 0, SYMBOL_CLASS, &data_string };
PREDEF(Binary, PREDEF_SYM_NAME(pathexpr)) = { 13, 0, SYMBOL_CLASS, &data_pathexpr };
PREDEF(Binary, PREDEF_SYM_NAME(printdepth)) = { 15, 0, SYMBOL_CLASS, &data_printdepth };
PREDEF(Binary, PREDEF_SYM_NAME(right)) = { 10, 0, SYMBOL_CLASS, &data_right };
PREDEF(Binary, PREDEF_SYM_NAME(top)) = { 8, 0, SYMBOL_CLASS, &data_top };
PREDEF(Binary, PREDEF_SYM_NAME(vars)) = { 9, 0, SYMBOL_CLASS, &data_vars };
PREDEF(Binary, PREDEF_SYM_NAME(_implementor)) = { 17, 0, SYMBOL_CLASS, &data__implementor };
PREDEF(Binary, PREDEF_SYM_NAME(_nextargframe)) = { 18, 0, SYMBOL_CLASS, &data__nextargframe };
PREDEF(Binary, PREDEF_SYM_NAME(_proto)) = { 11, 0, SYMBOL_CLASS, &data__proto };
PREDEF(Binary, PREDEF_SYM_NAME(_parent)) = { 12, 0, SYMBOL_CLASS, &data__parent };
char data_append[] = { 1, 0, 0, 0, 'a', 'p', 'p', 'e', 'n', 'd', 0 };
char data_array[] = { 1, 0, 0, 0, 'a', 'r', 'r', 'a', 'y', 0 };
char data_bottom[] = { 2, 0, 0, 0, 'b', 'o', 't', 't', 'o', 'm', 0 };
char data_call[] = { 3, 0, 0, 0, 'c', 'a', 'l', 'l', 0 };
char data_class[] = { 3, 0, 0, 0, 'c', 'l', 'a', 's', 's', 0 };
char data_cobj[] = { 3, 0, 0, 0, 'c', 'o', 'b', 'j', 0 };
char data_data[] = { 4, 0, 0, 0, 'd', 'a', 't', 'a', 0 };
char data_entry[] = { 5, 0, 0, 0, 'e', 'n', 't', 'r', 'y', 0 };
char data_errcode[] = { 5, 0, 0, 0, 'e', 'r', 'r', 'c', 'o', 'd', 'e', 0 };
char data_functions[] = { 6, 0, 0, 0, 'f', 'u', 'n', 'c', 't', 'i', 'o', 'n', 's', 0 };
char data_getstring[] = { 7, 0, 0, 0, 'g', 'e', 't', 's', 't', 'r', 'i', 'n', 'g', 0 };
char data_iterator[] = { 9, 0, 0, 0, 'i', 't', 'e', 'r', 'a', 't', 'o', 'r', 0 };
char data_left[] = { 12, 0, 0, 0, 'l', 'e', 'f', 't', 0 };
char data_numargs[] = { 14, 0, 0, 0, 'n', 'u', 'm', 'a', 'r', 'g', 's', 0 };
char data_real[] = { 2, 0, 0, 0, 'r', 'e', 'a', 'l', 0 };
char data_reset[] = { 2, 0, 0, 0, 'r', 'e', 's', 'e', 't', 0 };
char data_send[] = { 3, 0, 0, 0, 's', 'e', 'n', 'd', 0 };
char data_string[] = { 3, 0, 0, 0, 's', 't', 'r', 'i', 'n', 'g', 0 };
char data_pathexpr[] = { 0, 0, 0, 0, 'p', 'a', 't', 'h', 'e', 'x', 'p', 'r', 0 };
char data_printdepth[] = { 0, 0, 0, 0, 'p', 'r', 'i', 'n', 't', 'd', 'e', 'p', 't', 'h', 0 };
char data_right[] = { 2, 0, 0, 0, 'r', 'i', 'g', 'h', 't', 0 };
char data_top[] = { 4, 0, 0, 0, 't', 'o', 'p', 0 };
char data_vars[] = { 6, 0, 0, 0, 'v', 'a', 'r', 's', 0 };
char data__implementor[] = { 15, 0, 0, 0, '_', 'i', 'm', 'p', 'l', 'e', 'm', 'e', 'n', 't', 'o', 'r', 0 };
char data__nextargframe[] = { 15, 0, 0, 0, '_', 'n', 'e', 'x', 't', 'a', 'r', 'g', 'f', 'r', 'a', 'm', 'e', 0 };
char data__proto[] = { 15, 0, 0, 0, '_', 'p', 'r', 'o', 't', 'o', 0 };
char data__parent[] = { 15, 0, 0, 0, '_', 'p', 'a', 'r', 'e', 'n', 't', 0 };
void    InitPredefObjects()
{
    static Value predefSyms[] = {
        PSYM(append),
        PSYM(array),
        PSYM(bottom),
        PSYM(call),
        PSYM(class),
        PSYM(cobj),
        PSYM(data),
        PSYM(entry),
        PSYM(errcode),
        PSYM(functions),
        PSYM(getstring),
        PSYM(iterator),
        PSYM(left),
        PSYM(numargs),
        PSYM(real),
        PSYM(reset),
        PSYM(send),
        PSYM(string),
        PSYM(pathexpr),
        PSYM(printdepth),
        PSYM(right),
        PSYM(top),
        PSYM(vars),
        PSYM(_implementor),
        PSYM(_nextargframe),
        PSYM(_proto),
        PSYM(_parent),
    };

    InternPredefSyms(predefSyms, ARRAYSIZE(predefSyms));
}
