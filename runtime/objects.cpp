/*
    Proto language runtime

    Object system runtime

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include "objects-private.h"
#include "objhash.h"
#include "gc.h"
#include "predefined.h"
#include <string.h>

DECLARE_PSYM(_proto);
DECLARE_PSYM(array);
DECLARE_PSYM(cobj);
DECLARE_PSYM(data);
DECLARE_PSYM(errcode);
DECLARE_PSYM(pathexpr);
DECLARE_PSYM(real);
DECLARE_PSYM(string);

void    CheckFrame(Value frame);

//----------------------------------------------------------------
// Basic Value manipulation
//----------------------------------------------------------------

// Helper functions for V_xxx conversion inlines

#pragma warning(push)
#pragma warning(disable: 4702)  // unreachable code (returns after throws)

int     V_INT_error()
{
    PROTO_THROW(g_exType, E_NotAnInteger);
    return 0;
}

int     V_CHAR_error()
{
    PROTO_THROW(g_exType, E_NotACharacter);
    return 0;
}

Object* V_PTR_error()
{
    PROTO_THROW(g_exType, E_NotAPointer);
    return 0;
}

#pragma warning(pop)

// Internal function to find the Object corresponding to a pointer Value.
// Throws if v is not a pointer. Follows forwarding objects (created by
// ReplaceObject) if necessary.

Object* V_PTR(Value v)
{
    if (!V_ISPTR(v))
        V_PTR_error();
    Object* pObj = UNSAFE_V_PTR(v);
    while (pObj->flags & HDR_FORWARDER)
        pObj = pObj->pReplacement;
    return pObj;
}

Value   REAL_V(double d)
{
    return NewBinary(PSYM(real), &d, sizeof(double));
}

double  V_REAL(Value v)
{
    Object* pObj = V_PTR(v);
    if (!V_EQ(pObj->cls, PSYM(real)))
        PROTO_THROW(g_exType, E_NotAReal);
    return *(double*)pObj->pData;
}

bool    V_EQ(Value a, Value b)
{
    // The existence of ReplaceObject requires us to confirm that
    // two unequal pointers don't really refer to the same object.

    if (a == b)
        return true;
    else if (V_ISPTR(a) && V_ISPTR(b))
        return V_PTR(a) == V_PTR(b);
    else
        return false;
}

bool    IsBinary(Value obj)
{
    return V_ISPTR(obj) && (V_PTR(obj)->flags & HDR_SLOTTED) == 0;
}

bool    IsArray(Value obj)
{
    return V_ISPTR(obj) && (V_PTR(obj)->flags & (HDR_SLOTTED | HDR_FRAME)) == HDR_SLOTTED;
}

bool    IsFrame(Value obj)
{
    return V_ISPTR(obj) && (V_PTR(obj)->flags & (HDR_SLOTTED | HDR_FRAME)) == (HDR_SLOTTED | HDR_FRAME);
}

bool    IsSymbol(Value obj)
{
    return V_ISPTR(obj) && ObjIsSymbol(V_PTR(obj));
}

bool    IsString(Value obj)
{
    Object* pObj = V_PTR(obj);
    return (pObj->flags & HDR_SLOTTED) == 0 && pObj->cls == PSYM(string);
}

//----------------------------------------------------------------
// Binary objects
//----------------------------------------------------------------

/** Allocates a new Binary object.

    @arg @c cls Value of the object's @em class slot.
    @arg @c size The size of the object's data, in bytes.
*/

Value   NewBinary(Value cls, int size)
{
    Object* pObj = GC_NEW(Object);
    pObj->size = size;
    pObj->flags = 0;
    pObj->cls = cls;
    if (size > 0)
        pObj->pData = GC_MALLOC_ATOMIC(size);
    return PTR_V(pObj);
}

/** Allocates a new Binary object with the given data.

    @arg @c cls Value of the object's @em class slot.
    @arg @c pData Pointer to data to copy into the object.
    @arg @c size The size of the data, in bytes.
*/

Value   NewBinary(Value cls, void* pData, int size)
{
    Object* pObj = GC_NEW(Object);
    pObj->size = size;
    pObj->flags = 0;
    pObj->cls = cls;
    if (size > 0) {
        pObj->pData = GC_MALLOC_ATOMIC(size);
        memcpy(pObj->pData, pData, size);
    }
    return PTR_V(pObj);
}

//----------------------------------------------------------------
// Arrays
//----------------------------------------------------------------

/** Allocates a new Array. The slots will be initialized to @c V_NIL.

    @arg @c cls Value of the object's @em class slot.
    @arg @c nSlots Number of slots.
*/

Value   NewArray(Value cls, int nSlots)
{
    if (nSlots < 0 || nSlots > MAX_SLOTS)
        PROTO_THROW(g_exFr, E_BadArguments);
    Object* pObj = GC_NEW(Object);
    pObj->size = nSlots;
    pObj->flags = HDR_SLOTTED;
    pObj->cls = cls;
    if (nSlots > 0) {
        Value* pSlots = (Value*) GC_MALLOC(nSlots * sizeof(Value));
        pObj->pSlots = pSlots;
        for (int i = 0; i < nSlots; i++)
            pSlots[i] = V_NIL;
    }
    return PTR_V(pObj);
}

/** Allocates a new Array. The @em class slot will be initialized
    to @c Array. The slots will be initialized to @c V_NIL.

    @arg @c nSlots Number of slots.
*/

Value   NewArray(int nSlots)
{
    return NewArray(PSYM(array), nSlots);
}

/** Gets the value from the @c index-th slot of @c array.
*/

Value   GetSlot(Value array, int index)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);
    if (index < 0 || index >= (int) pObj->size)
        PROTO_THROW(g_exFr, E_OutOfBounds);
    return pObj->pSlots[index];
}

void*   GetData(Value binary)
{
    Object* pObj = V_PTR(binary);
    if ((pObj->flags & HDR_SLOTTED))
        PROTO_THROW(g_exType, E_NotABinary);
    return pObj->pData;
}

Value   GetClassSlot(Value obj)
{
    Object* pObj = V_PTR(obj);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) == (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_UnexpectedFrame);
    return pObj->cls;
}

void    SetClassSlot(Value obj, Value cls)
{
    Object* pObj = V_PTR(obj);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) == (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_UnexpectedFrame);
    pObj->cls = cls;
}

/** Puts @c newValue into the @c index-th slot of @c array.
*/

void    SetSlot(Value array, int index, Value newValue)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);
    if (index < 0 || index >= (int) pObj->size)
        PROTO_THROW(g_exFr, E_OutOfBounds);
    pObj->pSlots[index] = newValue;
}

// Changes the length of the given array or frame. Resizes pSlots
// to the given size and fills any new slots with V_NIL.

void    SetSlottedLength(Object* pObj, int nSlots)
{
    if (nSlots < 0 || nSlots > MAX_SLOTS)
        PROTO_THROW(g_exFr, E_OutOfBounds);

    int oldSize = pObj->size;
    if (nSlots == oldSize)
        return;

    Value* pSlots = (Value*) GC_REALLOC(pObj->pSlots, nSlots * sizeof(Value));
    pObj->pSlots = pSlots;
    if (nSlots > oldSize) {
        for (int i = oldSize; i < nSlots; i++)
            pSlots[i] = V_NIL;
    }
    pObj->size = nSlots;
}

// Increases the size of pObj->pSlots by one slot and puts newValue
// into the new slot.

void    AddSlotValue(Object* pObj, Value newValue)
{
    int nSlots = pObj->size + 1;
    Value* pSlots = (Value*) GC_REALLOC(pObj->pSlots, nSlots * sizeof(Value));
    pSlots[nSlots - 1] = newValue;
    pObj->size = nSlots;
    pObj->pSlots = pSlots;
}

void    SetArrayLength(Value array, int nSlots)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);
    SetSlottedLength(pObj, nSlots);
}

void    AddArraySlot(Value array, Value newValue)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);
    AddSlotValue(pObj, newValue);
}

Value*  GetArraySlots(Value array)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);

    return pObj->pSlots;
}

int     GetArrayLength(Value array)
{
    Object* pObj = V_PTR(array);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != HDR_SLOTTED)
        PROTO_THROW(g_exType, E_NotAnArray);

    return pObj->size;
}

void    SetBinaryLength(Value binary, int size)
{
    Object* pObj = V_PTR(binary);
    if ((pObj->flags & HDR_SLOTTED))
        PROTO_THROW(g_exType, E_NotABinary);
    if (size < 0 || size > MAX_DATA)
        PROTO_THROW(g_exFr, E_OutOfBounds);

    int oldSize = pObj->size;
    if (size == oldSize)
        return;

    pObj->pData = GC_REALLOC(pObj->pData, size);
    pObj->size = size;
}

int     GetBinaryLength(Value binary)
{
    Object* pObj = V_PTR(binary);
    if ((pObj->flags & HDR_SLOTTED))
        PROTO_THROW(g_exType, E_NotABinary);

    return pObj->size;
}

//----------------------------------------------------------------
// Frames
//----------------------------------------------------------------

// Size where maps transition to hash tables -- this should be set on a per-platform
// basis to the optimal point for performance. But at the moment it's 32, just because.

const int HASH_MAP_MIN = 32;

// Makes a new sequential frame map.

Value   NewMap(Value supermap)
{
    Value map = NewArray(SEQUENTIAL_MAP_CLASS, SeqMapArraySize(0));
    SetSlot(map, 0, supermap);
    return map;
}

// Gets the tag corresponding to the index-th slot of the given map,
// taking supermaps into account.
//
// TODO: Used by iterator code in interpreter--probably needs rethinking
//       given presence of hashed frames.

Value   GetMapTag(Value map, int index)
{
    Object* pMap = V_PTR(map);
    MapSlots* pMapSlots = (MapSlots*) (pMap->pSlots);

    if (pMapSlots->supermap != V_NIL) {
        Value v = GetMapTag(pMapSlots->supermap, index);
        if (V_ISINT(v))
            index -= V_INT(v);
        else
            return v;
    }

    int flags = UNSAFE_V_INT(pMap->cls);
    if (flags & HASH_MAP) {
        ASSERT(false);  // BUGBUG: NYI (and probably shouldn't be--need to fix iterator code instead)
        return V_NIL;
    }
    else {
        if (index < (int) pMap->size - 1)
            return pMapSlots->tags[index + 1];
        else
            return INT_V(pMap->size - 1);
    }
}

// Does a hash search on a hashed frame map. Finds the slot where the tag is,
// or else (optionally) finds the slot where the tag should be added.
// Returns true iff the tag was found.
//
// Table size must be a power of two. Uses double hashing.

bool    FindHashMapTag(int tableSize, MapSlots* pMapSlots, Value tag,
                       /* out */ int* pSlot, /* out */ int* pFreeSlot = 0)
{
    Object* pTag = V_PTR(tag);
    if (!ObjIsSymbol(pTag))
        PROTO_THROW_ERR(g_exType, E_NotASymbol, tag);

    int hash = ((SymbolData*) pTag->pData)->hash;
    int sizeMask = tableSize - 1;
    int bucket = hash & sizeMask;
    // incr is the double-hashing increment. Or-ing with 1 ensures
    // it is relatively prime to the tableSize (which is a power of two).
    int incr = ((hash * 13) & sizeMask) | 1;
    int freeSlot = -1;
    bool found;

    for (;;) {
        Value candidate = pMapSlots->hash.table[bucket];
        if (candidate == V_NIL) {
            freeSlot = bucket;
            found = false;
            break;
        }
        else if (V_EQ(candidate, tag)) {
            found = true;
            break;
        }
        else if (candidate == INT_V(0)) {
            freeSlot = bucket;
        }

        bucket = (bucket + incr) & sizeMask;
    }

    *pSlot = bucket;
    if (pFreeSlot != 0)
        *pFreeSlot = freeSlot;

    return found;
}

// Create a new hash map for the frame and rehash the existing map into it,
// resizing and rearranging the frame slots to correspond. The existing map
// may be hashed or sequential.
//
// newSize must be a power of two.

void    RehashFrame(Object* pFrame, int newSize)
{
    // Get our bearings

    int oldSize = pFrame->size;

    Value newMap = NewArray(HASH_MAP_CLASS, HashMapArraySize(newSize));
    MapSlots* newSlots = (MapSlots*) UNSAFE_V_PTR(newMap)->pSlots;
    Value* newData = (Value*) GC_MALLOC(newSize * sizeof(Value));

    MapSlots* oldSlots = (MapSlots*) UNSAFE_V_PTR(pFrame->map)->pSlots;
    Value* oldData = pFrame->pSlots;

    int nOccupied;
    Value* oldTags;

    int flags = UNSAFE_V_INT(UNSAFE_V_PTR(pFrame->map)->cls);
    if (flags & HASH_MAP) {
        nOccupied = UNSAFE_V_INT(oldSlots->hash.nOccupied);
        oldTags = oldSlots->hash.table;
    }
    else {
        nOccupied = pFrame->size;
        oldTags = oldSlots->tags;
    }

    // Copy old tags and slots to the proper places in the new map & data

    for (int i = 0; i < oldSize; i++) {
        Value tag = oldTags[i];

        if (tag != V_NIL && tag != INT_V(0)) {
            int iSlot;
            int iFreeSlot;
            bool found = FindHashMapTag(newSize, newSlots, tag, &iSlot, &iFreeSlot);
            ASSERT(!found && newSlots->hash.table[iFreeSlot] == V_NIL || newSlots->hash.table[iFreeSlot] == INT_V(0));

            newSlots->hash.table[iFreeSlot] = tag;
            newData[iFreeSlot] = oldData[i];
        }
    }

    // Set unused slots in new data to V_NIL

    for (int i = 0; i < newSize; i++) {
        if (newSlots->hash.table[i] == V_NIL)
            newData[i] = V_NIL;
    }

    newSlots->hash.nOccupied = INT_V(nOccupied);

    // Set frame to point to new stuff

    pFrame->map = newMap;
    pFrame->size = newSize;
    pFrame->pSlots = newData;
}

// This is mainly for use by streaming.cpp

EXPORT  Value   NewMapWithTags(Value tagArray)
{
    int nSlots = GetArrayLength(tagArray);
    Value map = NewArray(SEQUENTIAL_MAP_CLASS, SeqMapArraySize(nSlots));
    MapSlots* pMapSlots = (MapSlots*) V_PTR(map)->pSlots;
    memcpy(pMapSlots->tags, UNSAFE_V_PTR(tagArray)->pSlots, nSlots * sizeof(Value));
    return map;
}

Value   NewFrame(void)
{
    Object* pObj = GC_NEW(Object);
    pObj->size = 0;
    pObj->flags = HDR_SLOTTED | HDR_FRAME;
    pObj->map = NewMap(V_NIL);
    pObj->pSlots = 0;
    return PTR_V(pObj);
}

EXPORT  Value   NewFrameWithMap(Value map)
{
    Object* pMap = V_PTR(map);
    int flags = V_INT(pMap->cls);

    int nSlots;
    if (flags & HASH_MAP)
        nSlots = pMap->size - HashMapArraySize(0);
    else
        nSlots = pMap->size - SeqMapArraySize(0);

    Object* pObj = GC_NEW(Object);
    pObj->size = nSlots;
    pObj->flags = HDR_SLOTTED | HDR_FRAME;
    pObj->map = map;

    if (nSlots > 0) {
        Value* pSlots = (Value*) GC_MALLOC(nSlots * sizeof(Value));
        pObj->pSlots = pSlots;
        for (int i = 0; i < nSlots; i++)
            pSlots[i] = V_NIL;
    }
    else {
        pObj->pSlots = 0;
    }

    return PTR_V(pObj);
}

// Find the slot index corresponding to tag, counting supermap slots.
// (Jeff Piazza memorial recursive version)
//
// If tag is in map, returns a positive number which is the offset of
// tag in map plus the length of the supermaps -- that is, the offset
// in the data of the slot corresponding to the tag.
//
// If not, returns a negative number -(S+1) where S is the size of
// the map plus supermaps.
//
// I'm sure there's a less clever way to do this but it's a fond memory
// from the original code.

int     FindOffset(Value map, Value tag)
{
    Object* pMap = V_PTR(map);
    MapSlots* pMapSlots = (MapSlots*) (pMap->pSlots);

    int nPrevSlots;

    if (pMapSlots->supermap == V_NIL)
        nPrevSlots = 0;
    else {
        int index = FindOffset(pMapSlots->supermap, tag);
        if (index > 0)
            return index;
        else
            nPrevSlots = - index - 1;
    }

    int flags = UNSAFE_V_INT(pMap->cls);
    if (flags & HASH_MAP) {
        int tableSize = pMap->size - HashMapArraySize(0);
        int slot;
        if (FindHashMapTag(tableSize, pMapSlots, tag, &slot))
            return slot + nPrevSlots;
        else
            return - (nPrevSlots + tableSize + 1);
    }
    else {
        int nSlots = pMap->size - SeqMapArraySize(0);
        for (int i = 0; i < nSlots; i++) {
            if (V_EQ(tag, pMapSlots->tags[i]))
                return i + nPrevSlots;
        }
        return - (nPrevSlots + nSlots + 1);
    }
}

bool    HasSlot(Value frame, Value tag)
{
    Object* pObj = V_PTR(frame);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_NotAFrame);

    int index = FindOffset(pObj->map, tag);
    return (index >= 0);
}

Value   GetSlot(Value frame, Value tag)
{
    Object* pObj = V_PTR(frame);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_NotAFrame);
    int index = FindOffset(pObj->map, tag);
    if (index < 0)
        return V_NIL;
    else
        return pObj->pSlots[index];
}

// Internal function that does the real work of RemoveSlot.
//
// Life would be so much simpler if you couldn't remove slots from frames.
// This is built on the same recursive search algorithm as FindOffset. If
// the tag is found, we remove it from the map and data segment where we
// found it, in the appropriate hashed or sequential fashion.

int     RemoveSlotInner(Value map, Value tag, Object* pObj)
{
    Object* pMap = V_PTR(map);
    MapSlots* pMapSlots = (MapSlots*) pMap->pSlots;

    int nPrevSlots;

    if (pMapSlots->supermap == V_NIL)
        nPrevSlots = 0;
    else {
        int index = RemoveSlotInner(pMapSlots->supermap, tag, pObj);
        if (index > 0)
            return index;
        else
            nPrevSlots = - index - 1;
    }

    int flags = UNSAFE_V_INT(pMap->cls);
    if (flags & HASH_MAP) {
        int tableSize = pMap->size - HashMapArraySize(0);
        int slot;
        if (FindHashMapTag(tableSize, pMapSlots, tag, &slot)) {
            pMapSlots->hash.table[slot] = INT_V(0);
            pObj->pSlots[slot + nPrevSlots] = V_NIL;

            int nOccupied = UNSAFE_V_INT(pMapSlots->hash.nOccupied);
            pMapSlots->hash.nOccupied = INT_V(nOccupied - 1);

            // Shrink the hash table if enough data has been removed,
            // but don't let it get smaller than HASH_MAP_MIN.

            if (tableSize > HASH_MAP_MIN && nOccupied < tableSize / 4)
                RehashFrame(pObj, tableSize / 2);

            return slot + nPrevSlots;
        }
        else {
            return - (nPrevSlots + tableSize + 1);
        }
    }
    else {
        int nSlots = pMap->size - 1;
        for (int i = 0; i < nSlots; i++) {
            if (V_EQ(tag, pMapSlots->tags[i])) {
                // Slide everything below this slot up (in this map and in the data).
                // We don't reallocate the map or values, so the object still takes
                // up the same amount of space. This doesn't seem worth worrying about,
                // unless we have a lot of small frames with slot removal happening.
                memcpy(pMapSlots->tags + i, pMapSlots->tags + i + 1, sizeof(Value) * (nSlots - i - 1));
                memcpy(pObj->pSlots + i + nPrevSlots, pObj->pSlots + i + 1 + nPrevSlots,
                       sizeof(Value) * (pObj->size - i - nPrevSlots - 1));
                pObj->size--;
                pMap->size--;
                return i + nPrevSlots;
            }
        }
        return - (nPrevSlots + nSlots + 1);
    }
}

void    RemoveSlot(Value frame, Value tag)
{
    Object* pObj = V_PTR(frame);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_NotAFrame);

    RemoveSlotInner(pObj->map, tag, pObj);

#ifdef _DEBUG
    CheckFrame(frame);
#endif
}

// Chooses the right table size and hashes a frame.

void    ConvertToHashMap(Object* pFrame)
{
    int tableSize = 1;
    while (tableSize < pFrame->size)
        tableSize *= 2;

    if (pFrame->size > (tableSize/2 + tableSize/4))
        tableSize *= 2;

    RehashFrame(pFrame, tableSize);
}

// Do a sanity check on a frame and assert if something looks funny.

void    CheckFrame(Value frame)
{
    Object* pFrame = V_PTR(frame);
    Object* pMap = UNSAFE_V_PTR(pFrame->map);

    int flags = UNSAFE_V_INT(pMap->cls);
    if (flags & HASH_MAP) {
        MapSlots* pMapSlots = (MapSlots*) pMap->pSlots;
        int nSlots = pMap->size - HashMapArraySize(0);
        int nOccupied = UNSAFE_V_INT(pMapSlots->hash.nOccupied);
        for (int i = 0; i < nSlots; i++) {
            if (pMapSlots->hash.table[i] == V_NIL || pMapSlots->hash.table[i] == INT_V(0)) {
                ASSERT(pFrame->pSlots[i] == V_NIL);
            }
            else {
                nOccupied--;
            }
        }
        ASSERT(nOccupied == 0);
    }
    else {
        ASSERT(pFrame->size == pMap->size - SeqMapArraySize(0));
    }
}

// Adds a slot to a frame, converting to hashed frame if needed.
// Returns the index of the new slot in the frame data.

int     AddSlot(Object* pFrame, Value tag)
{
    Object* pMap = UNSAFE_V_PTR(pFrame->map);
    MapSlots* pMapSlots = (MapSlots*) pMap->pSlots;

    int flags = UNSAFE_V_INT(pMap->cls);

    if (flags & SHARED_MAP) {
        Value newMap = Clone(pFrame->map);
        pFrame->map = newMap;
        pMap = UNSAFE_V_PTR(newMap);
        pMap->cls = INT_V(UNSAFE_V_INT(pMap->cls) & ~SHARED_MAP);
        pMapSlots = (MapSlots*) pMap->pSlots;
    }

    if (flags & HASH_MAP) {
        int tableSize = pMap->size - HashMapArraySize(0);

        // Enlarge table if more than 3/4 full

        if (UNSAFE_V_INT(pMapSlots->hash.nOccupied) > (tableSize/2 + tableSize/4)) {
            tableSize *= 2;
            RehashFrame(pFrame, tableSize);
            pMap = UNSAFE_V_PTR(pFrame->map);
            pMapSlots = (MapSlots*) pMap->pSlots;
        }

        int slot;
        int freeSlot;
        bool exists = FindHashMapTag(tableSize, pMapSlots, tag, &slot, &freeSlot);
        ASSERT(!exists);

        pMapSlots->hash.table[freeSlot] = tag;
        pMapSlots->hash.nOccupied = INT_V(UNSAFE_V_INT(pMapSlots->hash.nOccupied) + 1);

        return freeSlot;
    }
    else {
        int nSlots = pFrame->size;

        if (nSlots >= HASH_MAP_MIN) {
            ConvertToHashMap(pFrame);
            return AddSlot(pFrame, tag);
        }

        SetSlottedLength(pFrame, nSlots + 1);
        AddSlotValue(pMap, tag);

        return nSlots;
    }
}

void    SetSlot(Value frame, Value tag, Value newValue)
{
    Object* pObj = V_PTR(frame);
    if ((pObj->flags & (HDR_SLOTTED | HDR_FRAME)) != (HDR_SLOTTED | HDR_FRAME))
        PROTO_THROW(g_exType, E_NotAFrame);

    int index = FindOffset(pObj->map, tag);

    if (index < 0)
        index = AddSlot(pObj, tag);

    pObj->pSlots[index] = newValue;

#ifdef _DEBUG
    CheckFrame(frame);
#endif
}

//----------------------------------------------------------------
// Misc.
//----------------------------------------------------------------

int     GetObjLength(Value obj)
{
    Object* pObj = V_PTR(obj);

    if (ObjIsFrame(pObj)) {
        Object* pMap = UNSAFE_V_PTR(pObj->map);

        int flags = UNSAFE_V_INT(pMap->cls);
        if (flags & HASH_MAP) {
            MapSlots* pMapSlots = (MapSlots*) pMap->pSlots;
            return UNSAFE_V_INT(pMapSlots->hash.nOccupied);
        }
        else {
            return pMap->size - SeqMapArraySize(0);
        }
    }
    else {
        return V_PTR(obj)->size;
    }
}

void    ReplaceObject(Value oldObj, Value newObj)
{
    Object* pOld = V_PTR(oldObj);
    Object* pNew = V_PTR(newObj);
    pOld->size = ~0;
    pOld->flags = HDR_FORWARDER;
    pOld->pReplacement = pNew;
    pOld->pData = 0;
}

Value   Clone(Value obj)
{
    if (!V_ISPTR(obj))
        return obj;

    Object* pObj = V_PTR(obj);
    int size = pObj->size;

    Object* pNew = GC_NEW(Object);

    pNew->size = size;
    pNew->flags = pObj->flags;

    if (ObjIsFrame(pObj)) {
        // Mark the map as shared
        Value& rCls = UNSAFE_V_PTR(pObj->map)->cls;
        rCls = INT_V(UNSAFE_V_INT(rCls) | SHARED_MAP);
        pNew->map = pObj->map;
    }
    else {
        pNew->cls = pObj->cls;
    }

    if (size > 0) {
        if (pObj->flags & HDR_SLOTTED) {
            int numBytes =  size * sizeof(Value);
            pNew->pSlots = (Value*) GC_MALLOC(numBytes);
            memcpy(pNew->pSlots, pObj->pSlots, numBytes);
        }
        else {
            pNew->pData = GC_MALLOC_ATOMIC(size);
            memcpy(pNew->pData, pObj->pData, size);
        }
    }
    else {
        pNew->pData = 0;
    }

    return PTR_V(pNew);
}

Value   DeepCloneInner(Value obj, ObjHashTable<Value>& hash)
{
    ASSERT(V_ISPTR(obj));

    // Symbols don't get cloned
    if (IsSymbol(obj))
        return obj;

    Value newObj;

    if (hash.Get(obj, &newObj))
        return newObj;

    newObj = Clone(obj);

    hash.Set(obj, newObj);

    Object* pNewObj = UNSAFE_V_PTR(newObj);

    if (pNewObj->flags & HDR_SLOTTED) {
        // Clone array class slot (frame maps are shared, not cloned)
        if (ObjIsArray(pNewObj) && V_ISPTR(pNewObj->cls))
            pNewObj->cls = DeepCloneInner(pNewObj->cls, hash);

        // Clone slots
        for (int i = 0; i < pNewObj->size; i++) {
            Value v = pNewObj->pSlots[i];
            if (V_ISPTR(v))
                pNewObj->pSlots[i] = DeepCloneInner(v, hash);
        }
    }
    else {
        // Clone binary class slot
        if (V_ISPTR(pNewObj->cls))
            pNewObj->cls = DeepCloneInner(pNewObj->cls, hash);
    }

    return newObj;
}

Value   DeepClone(Value obj)
{
    if (V_ISPTR(obj)) {
        ObjHashTable<Value> hash;

        return DeepCloneInner(obj, hash);
    }
    else {
        return obj;
    }
}

Value   GetPath(Value obj, Value path)
{
    if (V_ISINT(path)) {
        return GetSlot(obj, UNSAFE_V_INT(path));
    }
    else {
        Object* pPath = V_PTR(path);

        if (ObjIsSymbol(pPath)) {
            return GetSlot(obj, path);
        }
        else if (ObjIsArray(pPath) && V_EQ(pPath->cls, PSYM(pathexpr))) {
            Value curObj = obj;

            int nSlots = pPath->size;

            for (int i = 0; i < nSlots; i++) {
                Value pathElt = pPath->pSlots[i];
                if (V_ISINT(pathElt))
                    curObj = GetSlot(curObj, UNSAFE_V_INT(pathElt));
                else
                    curObj = GetSlot(curObj, pathElt);
            }

            return curObj;
        }
        else {
            PROTO_THROW_ERR(g_exType, E_InvalidPath, path);
        }
    }
}

void    SetPath(Value obj, Value path, Value newValue)
{
    if (V_ISINT(path)) {
        SetSlot(obj, UNSAFE_V_INT(path), newValue);
    }
    else {
        Object* pPath = V_PTR(path);

        if (ObjIsSymbol(pPath)) {
            SetSlot(obj, path, newValue);
        }
        else if (ObjIsArray(pPath) && V_EQ(pPath->cls, PSYM(pathexpr))) {
            Value curObj = obj;
            Value pathElt;

            int nSlots = pPath->size;
            if (nSlots == 0)
                PROTO_THROW_ERR(g_exType, E_InvalidPath, path);

            for (int i = 0; i < nSlots - 1; i++) {
                pathElt = pPath->pSlots[i];
                if (V_ISINT(pathElt))
                    curObj = GetSlot(curObj, UNSAFE_V_INT(pathElt));
                else
                    curObj = GetSlot(curObj, pathElt);
            }

            pathElt = pPath->pSlots[nSlots - 1];
            if (V_ISINT(pathElt))
                SetSlot(curObj, UNSAFE_V_INT(pathElt), newValue);
            else
                SetSlot(curObj, pathElt, newValue);
        }
        else {
            PROTO_THROW_ERR(g_exType, E_InvalidPath, path);
        }
    }
}

bool    HasPath(Value obj, Value path)
{
    if (!V_ISPTR(obj))
        return false;

    if (V_ISINT(path)) {
        UInt index = UNSAFE_V_INT(path);
        Object* pObj = V_PTR(obj);
        if (ObjIsArray(pObj) && index >= 0 && index < pObj->size)
            return true;
        else
            return false;
    }
    else {
        Object* pPath = V_PTR(path);

        if (ObjIsSymbol(pPath)) {
            return ObjIsFrame(UNSAFE_V_PTR(obj)) && HasSlot(obj, path);
        }
        else if (ObjIsArray(pPath) && V_EQ(pPath->cls, PSYM(pathexpr))) {
            Value curObj = obj;

            int nSlots = pPath->size;

            for (int i = 0; i < nSlots; i++) {
                Value pathElt = pPath->pSlots[i];
                if (V_ISPTR(curObj)) {
                    Object* pCurObj = UNSAFE_V_PTR(curObj);

                    if (V_ISINT(pathElt)) {
                        UInt index = UNSAFE_V_INT(pathElt);
                        if (ObjIsArray(pCurObj) &&
                                index >= 0 && index < pCurObj->size)
                            curObj = pCurObj->pSlots[UNSAFE_V_INT(pathElt)];
                        else
                            return false;
                    }
                    else if (ObjIsFrame(pCurObj) &&
                            ObjIsSymbol(UNSAFE_V_PTR(pathElt)))
                        curObj = GetSlot(curObj, pathElt);
                    else
                        return false;
                }
                else {
                    return false;
                }
            }

            return true;
        }
        else {
            PROTO_THROW_ERR(g_exType, E_InvalidPath, path);
        }
    }
}

/// Proto exceptions are represented by the ProtaException structure. The details
/// of exceptions are done this way for NewtonScript compatibility, not because it's
/// a particularly good approach!

ProtaException::ProtaException(const char* theName, int err, Value theData)
        : name(theName)
{
    Value exObj = NewFrame();   // BUGBUG: use predef map
    SetSlot(exObj, PSYM(errcode), INT_V(err));
    SetSlot(exObj, PSYM(data), theData);
    data = exObj;
}

// Is name1 a subexception of name2?

bool    IsSubexception(const char* name1, const char* name2)
{
    // BUGBUG: doesn't understand ;
    int len1 = strlen(name1);
    int len2 = strlen(name2);
    if (len1 < len2)
        return false;
    if (strncmp(name1, name2, len2) == 0 && (name1[len2] == '.' || name1[len2] == '\0'))
        return true;
    return false;
}

Value   NewString(int size)
{
    return NewBinary(PSYM(string), size);
}

Value   NewString(const TCHAR* str)
{
    Value result = NewBinary(PSYM(string), (_tcslen(str) + 1) * sizeof(TCHAR));
    _tcscpy((TCHAR*) GetData(result), str);
    return result;
}

TCHAR*  GetCString(Value str)
{
    return (TCHAR*) GetData(str);
}

void    StrAppend(Value str1, Value str2)
{
    if (!IsString(str1))
        PROTO_THROW(g_exType, E_NotAString);
    if (!IsString(str2))
        PROTO_THROW(g_exType, E_NotAString);

    int str1Len = GetBinaryLength(str1) / sizeof(wchar_t);
    int str2Len = GetBinaryLength(str2) / sizeof(wchar_t);
    SetBinaryLength(str1, (str1Len + str2Len - 1) * sizeof(wchar_t));
    wchar_t* pstr1 = (wchar_t*) GetData(str1);
    memcpy(pstr1 + str1Len - 1, GetData(str2), (str2Len - 1) * sizeof(wchar_t));
    pstr1[str1Len + str2Len - 2] = 0;
}

// BUGBUG: On Win32, should just do this in DLLMain

void    InitProtoLib()
{
    extern void InitPredefObjects(void);
    extern void InitInterpreter(void);

    InitPredefObjects();
    InitInterpreter();
}
