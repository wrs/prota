/*
	Prota language runtime

	Object hash table

	Copyright 2003 Walter R. Smith
	Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __OBJHASH_H__
#define __OBJHASH_H__

#include "gc.h"

// Hash table with Value keys (uses Value itself as the key--pointer identity).
// ObjHashTable<type> maps Value to type

template<typename VALUE_T>
class ObjHashTable {
public:
    ObjHashTable()
    {
        m_size = 0;
        
        // Start out using a stack buffer--we'll switch to heap if it overflows.

        m_table = m_initialTable;
        m_capacity = ARRAYSIZE(m_initialTable);
        m_mask = m_capacity - 1;
        memset(m_table, 0, m_capacity * sizeof(Element));
    }

    bool    Get(Value key, /*out*/ VALUE_T* pValue)
    {
        int iSlot;
        if (Find(key, &iSlot)) {
            *pValue = m_table[iSlot].value;
            return true;
        }
        else {
            return false;
        }
    }

    void    Set(Value key, const VALUE_T& value)
    {
        int iSlot;
        if (Find(key, &iSlot)) {
            m_table[iSlot].value = value;
        }
        else {
            if (m_size >= m_capacity / 2 + m_capacity / 4)
                Resize(m_capacity * 2);

            m_table[iSlot].key = key;
            m_table[iSlot].value = value;
            m_size++;
        }
    }

    bool    HasKey(Value key)
    {
        int iSlot;
        return Find(key, &iSlot);
    }

private:
    struct Element {
        Value   key;
        VALUE_T value;
    };

    bool    Find(Value key, int* iSlot)
    {
        int hash = ((UInt32) key * 0x9e3779b9) & m_mask;
        int incr = ((hash * 13) & m_mask) | 1;
        for (;;) {
            Value k = m_table[hash].key;
            if (k == 0) {
                *iSlot = hash;
                return false;
            }
            else if (V_EQ(k, key)) {
                *iSlot = hash;
                return true;
            }

            hash = (hash + incr) & m_mask;
        }

        ASSERT(false);
        return false;
    }

    void    SetCapacity(int newCapacity)
    {
        m_capacity = newCapacity;
        m_mask = m_capacity - 1;
        m_table = (Element*) GC_MALLOC(m_capacity * sizeof(Element));
        memset(m_table, 0, m_capacity * sizeof(Element));
    }

    void    Resize(int newCapacity)
    {
        int oldCapacity = m_capacity;
        Element* oldTable = m_table;
        
        SetCapacity(m_capacity * 2);

        for (int i = 0; i < oldCapacity; i++) {
            if (oldTable[i].key != 0)
                Set(oldTable[i].key, oldTable[i].value);
        }
    }

    int     m_size;
    int     m_capacity;
    int     m_mask;
    Element* m_table;
    Element m_initialTable[16];     // Must be a power of two
};

#endif // __OBJHASH_H__
