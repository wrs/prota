/*
	Proto language runtime

	Private interface to the object system

	Copyright 1997-1999 Walter R. Smith
	Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __OBJECTS_PRIVATE_H__
#define __OBJECTS_PRIVATE_H__

#include "stddef.h"
#include "objects.h"

struct Object {
	UInt32	size : 28;
	UInt32	flags : 4;
	union {
		Value	cls;
		Value	map;
		Object*	pReplacement;
	};
	union {
		void*	pData;
		Value*	pSlots;
	};
};

enum {
	HDR_SLOTTED = 1,
	HDR_FRAME = 2,
	HDR_FORWARDER = 4
};

const Value	FUNCTION_CLASS  = IMMED_V(IMMED_SPECIAL, 0x3);		// a.k.a. 0x32
const Value	SYMBOL_CLASS    = IMMED_V(IMMED_SPECIAL, 0x5555);	// a.k.a. 0x55552
const Value NATIVE_FN_CLASS = IMMED_V(IMMED_SPECIAL, 4);	    // a.k.a. 0x42

inline bool ObjIsFrame(Object* pObj)
	{ return (pObj->flags & (HDR_SLOTTED | HDR_FRAME)) == (HDR_SLOTTED | HDR_FRAME); }

inline bool ObjIsArray(Object* pObj)
	{ return (pObj->flags & (HDR_SLOTTED | HDR_FRAME)) == HDR_SLOTTED; }

inline bool ObjIsBinary(Object* pObj)
	{ return (pObj->flags & HDR_SLOTTED) == 0; }

inline bool ObjIsSymbol(Object* pObj)
	{ return (pObj->flags & HDR_SLOTTED) == 0 && pObj->cls == SYMBOL_CLASS; }

const int MAX_SLOTS = (1 << 28) - 1;
const int MAX_DATA = (1 << 28) - 1;

inline Value PTR_V(void* p) { return (Value) (((int) p) | TAG_PTR); }
Object* V_PTR(Value v);
inline Object* UNSAFE_V_PTR(Value v) { return (Object*) (((int) v) - 1); }

// Map class flags (for the cls slot of map objects)

enum {
    SORTED_MAP = 1,
    HASH_MAP = 2,
    HAS_PROTO = 4,
    SHARED_MAP = 8
};

const Value SEQUENTIAL_MAP_CLASS = INT_V(0);
const Value HASH_MAP_CLASS = INT_V(HASH_MAP);

struct MapSlots {
	Value	supermap;
    union {
        Value	tags[1];        // for sequential maps
        struct {                // for hash maps
            Value   nOccupied;
            Value   table[1];
        } hash;
    };
};

inline int SeqMapArraySize(int nSlots) { return offsetof(MapSlots, tags[nSlots]) / sizeof(Value); }
inline int HashMapArraySize(int nSlots) { return offsetof(MapSlots, hash.table[nSlots]) / sizeof(Value); }

void	SetSlottedLength(Object* pObj, int nSlots);

Value	GetMapTag(Value map, int index);

struct SymbolData {
	int		hash;
	char	name[1];
};

void	InternPredefSyms(Value syms[], int len);

#endif //__OBJECTS_PRIVATE_H__
