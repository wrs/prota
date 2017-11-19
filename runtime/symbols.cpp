/*
	Proto language runtime

	Symbol handling

	Copyright 1997-1999 Walter R. Smith
	Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include "objects-private.h"
#include "gc.h"
#include <string.h>

// BUGBUG: should be replaced with double hashing

struct Bucket {
	Bucket*		next;
	Value		sym;
};

Bucket*		g_buckets[16];

int		SymbolHash(const char* name)
{
	int hash = 0;
    while (*name) {
        hash = hash * 67 + toupper(*name) - 113;     // As used by GCC
        name++;
    }
    return hash;
}

void	InternPredefSyms(Value syms[], int len)
{
	for (int i = 0; i < len; i++) {
		SymbolData* pSymData = (SymbolData*) GetData(syms[i]);
        // BUGBUG: Hash should be precalculated
        pSymData->hash = SymbolHash(pSymData->name);
		int bucket = pSymData->hash & (ARRAYSIZE(g_buckets) - 1);
		Bucket* pBucket = GC_NEW(Bucket);
		pBucket->next = g_buckets[bucket];
		pBucket->sym = syms[i];
		g_buckets[bucket] = pBucket;
	}
}

Value	Intern(const char* name)
{
	int len = offsetof(SymbolData, name[0]) + strlen(name) + 1;
	int hash = SymbolHash(name);
	int bucket = hash & (ARRAYSIZE(g_buckets) - 1);
	
	Bucket* pBucket = g_buckets[bucket];
	while (pBucket) {
		Value sym = pBucket->sym;
		if (GetBinaryLength(sym) == len) {
			SymbolData* pSymData = (SymbolData*) GetData(sym);
			if (pSymData->hash == hash && !_stricmp(pSymData->name, name))
				return sym;
		}
		pBucket = pBucket->next;
	}

	Value sym = NewBinary(SYMBOL_CLASS, len);
	SymbolData* pSymData = (SymbolData*) GetData(sym);
	pSymData->hash = hash;
	strcpy(pSymData->name, name);

	pBucket = GC_NEW(Bucket);
	pBucket->next = g_buckets[bucket];
	pBucket->sym = sym;
	g_buckets[bucket] = pBucket;

	return sym;
}

const char*	SymbolName(Value sym)
{
	if (GetClassSlot(sym) != SYMBOL_CLASS)
		PROTO_THROW(g_exType, E_NotASymbol);
	SymbolData* pSymData = (SymbolData*) GetData(sym);
	return pSymData->name;
}
