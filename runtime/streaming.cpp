/*
    Prota language runtime

    Stream file I/O

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#include "config.h"
#include "objects-private.h"
#include "predefined.h"
#include <iostream>
#include <fstream>

using namespace std;

DECLARE_PSYM(array);
DECLARE_PSYM(bottom);
DECLARE_PSYM(left);
DECLARE_PSYM(real);
DECLARE_PSYM(right);
DECLARE_PSYM(string);
DECLARE_PSYM(top);

enum {
    T_IMMED,
    T_CHAR,
    T_UNICHAR,
    T_BINARY,
    T_ARRAY,
    T_PLAINARRAY,
    T_FRAME,
    T_SYMBOL,
    T_STRING,
    T_PRECEDENT,
    T_NIL,
    T_SMALLRECT,
    T_LARGEBINARY
};

class CPrecedents {
public:
    CPrecedents();

    int         Add(Value v);
    Value       Get(int index);
    void        Set(int index, Value v);

private:
    Value       m_list;
    int         m_size;
    int         m_pos;

    enum { SIZE_INCR = 128 };
};

CPrecedents::CPrecedents()
{
    m_list = NewArray(SIZE_INCR);
    m_size = SIZE_INCR;
    m_pos = 0;
}

int     CPrecedents::Add(Value v)
{
    if (m_pos == m_size)
        SetArrayLength(m_list, m_size + SIZE_INCR);
    SetSlot(m_list, m_pos, v);
    int index = m_pos;
    m_pos++;
    return index;
}

Value   CPrecedents::Get(int index)
{
    if (index < 0 || index >= m_pos)
        PROTA_THROW(g_exFr, E_BadStreamFormat);
    return GetSlot(m_list, index);
}

void    CPrecedents::Set(int index, Value v)
{
    SetSlot(m_list, index, v);
}

class CObjectReader {
public:
    CObjectReader(istream& in);

    Value       Read(void);

private:
    Value       ReadOne(void);
    int         ReadXLong(void);

    istream&    m_in;
    CPrecedents m_precedents;

    CObjectReader& operator=(CObjectReader&) { ASSERT(false); }
};

CObjectReader::CObjectReader(istream& in)
    : m_in(in)
{
}

Value   CObjectReader::Read()
{
    Byte version;
    m_in >> version;
    if (version != 1 && version != 2)
        PROTA_THROW(g_exFr, E_BadStreamFormat);

    return ReadOne();
}

Value   CObjectReader::ReadOne()
{
    int i;
    int slot;
    Byte b;
    Value v;
    Value cls;

    Byte op;
    m_in >> op;

    if (m_in.eof())
        PROTA_THROW(g_exFr, E_BadStreamFormat);

    switch (op) {
    case T_IMMED:
        i = ReadXLong();
        return (Value) i;

    case T_CHAR:
        m_in >> b;
        return CHAR_V(b);

    case T_UNICHAR:
        m_in >> b;
        i = b;
        m_in >> b;
        i = (i << 8) | b;
        return CHAR_V(b);

    case T_BINARY:
        i = ReadXLong();
        v = NewBinary(V_NIL, i);
        m_precedents.Add(v);
        cls = ReadOne();
        SetClassSlot(v, cls);
        m_in.read((char*) GetData(v), i);
        return v;

    case T_ARRAY:
        i = ReadXLong();
        v = NewArray(V_NIL, i);
        m_precedents.Add(v);
        cls = ReadOne();
        SetClassSlot(v, cls);
        for (slot = 0; slot < i; slot++)
            SetSlot(v, slot, ReadOne());
        return v;

    case T_PLAINARRAY:
        i = ReadXLong();
        v = NewArray(i);
        m_precedents.Add(v);
        for (slot = 0; slot < i; slot++)
            SetSlot(v, slot, ReadOne());
        return v;

    case T_FRAME:
        {
            // It has to _seem_ like we put the frame into precedents
            // before reading the tags, but since tags have to be symbols,
            // there's no harm in faking it until a more convenient time.
            int framePrecedent = m_precedents.Add(V_NIL);

            i = ReadXLong();
            Value tags = NewArray(i);
            for (slot = 0; slot < i; slot++)
                SetSlot(tags, slot, ReadOne());
            v = NewFrameWithMap(NewMapWithTags(tags));
            m_precedents.Set(framePrecedent, v);
            Value* pSlots = V_PTR(v)->pSlots;
            for (slot = 0; slot < i; slot++)
                pSlots[slot] = ReadOne();
            return v;
        }

    case T_SYMBOL:
        {
            char name[256];
            i = ReadXLong();
            if (i > ARRAYSIZE(name) - 2)
                PROTA_THROW(g_exFr, E_BadStreamFormat);
            m_in.read(name, i);
            name[i] = '\0';
            v = Intern(name);
            m_precedents.Add(v);
            return v;
        }

    case T_STRING:
        {
            i = ReadXLong();
            v = NewBinary(PSYM(string), i);
            m_precedents.Add(v);
            m_in.read((char*) GetData(v), i);
            SWAB((char*) GetData(v), (char*) GetData(v), i);
            return v;
        }

    case T_PRECEDENT:
        i = ReadXLong();
        return m_precedents.Get(i);

    case T_NIL:
        return V_NIL;

    case T_SMALLRECT:
        // BUGBUG: use predefined frame
        v = NewFrame();
        m_precedents.Add(v);
        m_in >> b;
        SetSlot(v, PSYM(top), INT_V(b));
        m_in >> b;
        SetSlot(v, PSYM(left), INT_V(b));
        m_in >> b;
        SetSlot(v, PSYM(bottom), INT_V(b));
        m_in >> b;
        SetSlot(v, PSYM(right), INT_V(b));
        return v;

    case T_LARGEBINARY:
        PROTA_THROW(g_exFr, E_BadStreamFormat);

    default:
        PROTA_THROW(g_exFr, E_BadStreamFormat);
    }
}

int     CObjectReader::ReadXLong()
{
    Byte b;
    int i;
    m_in >> b;
    if (b == 255) {
        m_in >> b;
        i = b;
        m_in >> b;
        i = (i << 8) | b;
        m_in >> b;
        i = (i << 8) | b;
        m_in >> b;
        i = (i << 8) | b;
        return i;
    }
    else
        return b;
}


Value   ReadStreamFile(const char* filename)
{
    ifstream stream(filename, ios::binary);
    stream.unsetf(ios::skipws); // why does this get set ??
    if (stream.fail())
        PROTA_THROW(g_exFr, E_BadStreamFormat);
    CObjectReader reader(stream);
    return reader.Read();
}
