/*
	Proto language runtime

	Printer

	Copyright 1997-1999 Walter R. Smith
	Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include "objects-private.h"
#include "interpreter.h"
#include "predefined.h"
#include "native.h"
#include "objhash.h"
#include <stdio.h>

DECLARE_PSYM(array);
DECLARE_PSYM(pathexpr);
DECLARE_PSYM(printdepth);
DECLARE_PSYM(real);
DECLARE_PSYM(string);

typedef int (*PrintFnPtr)(const char* format, ...);

void	PrintOneValue(Value v, int depth, PrintFnPtr printFn);

struct Printer {
	Printer(int maxDepth, PrintFnPtr printFn)
		{ m_maxDepth = maxDepth; m_printFn = printFn; m_depth = 0; }

	int		PrintFrameSlots(Value* pSlots, Value map, bool first = true);
	void	PrintOneValue(Value v);

	int			m_maxDepth;
	PrintFnPtr	m_printFn;
	int			m_depth;
    ObjHashTable<bool> m_hash;
};

int		Printer::PrintFrameSlots(Value* pSlots, Value map, bool first)
{
	Object* pMap = V_PTR(map);
	MapSlots* pMapSlots = (MapSlots*) (pMap->pSlots);
	
	int offset;
	if (pMapSlots->supermap == V_NIL)
		offset = 0;
	else
		offset = PrintFrameSlots(pSlots, pMapSlots->supermap, false);
	
    int nSlots;

    int flags = UNSAFE_V_INT(pMap->cls);
    if (flags & HASH_MAP) {
        nSlots = UNSAFE_V_INT(pMapSlots->hash.nOccupied);
        int nPrinted = 0;

        int tableSize = pMap->size - HashMapArraySize(0);
        for (int i = 0; i < tableSize; i++) {
            Value tag = pMapSlots->hash.table[i];
            if (tag != V_NIL && tag != INT_V(0)) {
		        PrintOneValue(tag);
		        (*m_printFn)(": ");
		        PrintOneValue(pSlots[offset + i]);
		        if (!(first && (nPrinted == nSlots - 1)))
			        (*m_printFn)(", ");
                nPrinted++;
            }
        }
    }
    else {
	    nSlots = pMap->size - 1;
	    for (int i = 0; i < nSlots; i++) {
		    PrintOneValue(pMapSlots->tags[i]);
		    (*m_printFn)(": ");
		    PrintOneValue(pSlots[offset + i]);
		    if (!(first && (i == nSlots - 1)))
			    (*m_printFn)(", ");
	    }
    }

	return offset + nSlots;
}

void	Printer::PrintOneValue(Value v)
{
	m_depth++;

	switch (V_TAG(v)) {
	case TAG_INT:
		(*m_printFn)("%d", V_INT(v));
		break;

	case TAG_PTR:
		{
			Object* pObj = UNSAFE_V_PTR(v);
            
            // Don't get into cycles
            if (m_hash.HasKey(v)) {
                if (ObjIsBinary(pObj))
                    (*m_printFn)("<#%X>", (int) v);
                else if (ObjIsArray(pObj))
                    (*m_printFn)("[#%X]", (int) v);
                else if (ObjIsFrame(pObj))
                    (*m_printFn)("{#%X}", (int) v);
            }
            else {
                // BUGBUG: Shouldn't do strings either
                if (!ObjIsSymbol(pObj))
                    m_hash.Set(v, true);

			    int flags = pObj->flags;
			    if (flags & HDR_FORWARDER) {
				    (*m_printFn)("-> ");
				    PrintOneValue(PTR_V(pObj->pReplacement));
			    }
			    else if (flags & HDR_SLOTTED) {
				    if (flags & HDR_FRAME) {
					    (*m_printFn)("{");
					    if (m_depth <= m_maxDepth)
						    PrintFrameSlots(pObj->pSlots, pObj->map);
					    else
						    (*m_printFn)("%X", (int) v);
					    (*m_printFn)("}");
				    }
                    else if (V_EQ(pObj->cls, PSYM(pathexpr))) {
                        int nSlots = pObj->size;
                        for (int i = 0; i < nSlots; i++) {
                            PrintOneValue(pObj->pSlots[i]);
                            if (i != nSlots - 1)
                                (*m_printFn)(".");
                        }
                    }
				    else {
					    (*m_printFn)("[");
					    if (m_depth <= m_maxDepth) {
						    if (!V_EQ(pObj->cls, PSYM(array))) {
							    PrintOneValue(pObj->cls);
							    (*m_printFn)(": ");
						    }
						    int nSlots = pObj->size;
						    for (int i = 0; i < nSlots; i++) {
							    PrintOneValue(pObj->pSlots[i]);
							    if (i != nSlots - 1)
								    (*m_printFn)(", ");
						    }
					    }
					    else
						    (*m_printFn)("%X", (int) v);
					    (*m_printFn)("]");
				    }
			    }
			    else {
				    ASSERT(!(flags & HDR_FRAME));
				    Value cls = pObj->cls;
				    if (cls == SYMBOL_CLASS)
					    (*m_printFn)("%s", SymbolName(v));
				    else if (V_EQ(cls, PSYM(real)))
					    (*m_printFn)("%lf", V_REAL(v));
				    else if (V_EQ(cls, PSYM(string))) {
					    (*m_printFn)("\"");
					    int len = pObj->size / sizeof(TCHAR) - 1;
					    TCHAR* p = (TCHAR*) pObj->pData;
					    for (int i = 0; i < len; i++) {
						    if (p[i] >= 32 && p[i] <= 127)
							    (*m_printFn)("%c", p[i]);
						    else if (p[i] == 13)
							    (*m_printFn)("\\n");
						    else
							    (*m_printFn)("*");
					    }
					    (*m_printFn)("\"");
				    }
				    else {
					    (*m_printFn)("<");
					    PrintOneValue(pObj->cls);
					    (*m_printFn)(" %d bytes>", pObj->size);
				    }
			    }
            }
		}
		break;

	case TAG_IMMED:
		if (v == V_NIL)
			(*m_printFn)("nil");
		else if (v == V_TRUE)
			(*m_printFn)("true");
		else if (V_ISCHAR(v))
			(*m_printFn)("$%c", V_CHAR(v));
		else
			(*m_printFn)("#%X", (int) v);
		break;

	case TAG_MAGICPTR:
		(*m_printFn)("#%X", (int) v);
		break;
	}

	m_depth--;
}

#ifdef _DEBUG
void	dbgPrintOneValue(Value v, int depth)
{
	Printer prt(depth, dbgprintf);
	prt.PrintOneValue(v);
}
#endif

void	PrintValue(Value v)
{
	int maxDepth;
	Value max;
	if (GetGlobalVar(PSYM(printdepth), &max) && V_ISINT(max))
		maxDepth = V_INT(max);
	else
		maxDepth = 0x7FFFFFFF;

	Printer prt(maxDepth, printf);
	prt.PrintOneValue(v);
}

void	PrintValueLn(Value v)
{
	PrintValue(v);
	printf("\n");
}

void    SetMaxPrintDepth(int maxDepth)
{
    SetGlobalVar(PSYM(printdepth), INT_V(maxDepth));
}

NATIVE_FUNC(FPrint)
{
	NATIVE_ARGS_1(value);

	PrintValueLn(ARG(value));

	return V_NIL;
}

DECLARE_GLOBAL_FUNCTION("Print", FPrint, 1);

