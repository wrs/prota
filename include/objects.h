/*
    Proto language runtime

    Object system interfaces

    Copyright 1997-2000 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __OBJECTS_H__
#define __OBJECTS_H__

/** @file */

#include "config.h"
#include "proto-errors.h"

/// Fake struct for Value typechecking.
/// This is just fakery so the compiler will do @c int vs. @c Value
/// typechecking/overloading for us. @c Values are really @c ints.
/// If you get a compiler error regarding the nonexistent <tt>struct _VALUE</tt>,
/// you've probably passed an @c int where a @c Value was required, or vice versa.

typedef struct _VALUE* Value;

const int TAG_MASK = 3;
enum {
    TAG_INT,
    TAG_PTR,
    TAG_IMMED,
    TAG_MAGICPTR
};

const int IMMED_MASK = 0xC;
enum {
    IMMED_SPECIAL = 0,
    IMMED_CHAR = 4,
    IMMED_BOOLEAN = 8
};

#define MAKEVALUE(val, tag) ((Value) (((val) << 2) | tag))

#define IMMED_V(type, val) ((Value) (((val) << 4) | (type) | TAG_IMMED))

/// NIL, nada, nothing

const Value V_NIL = IMMED_V(IMMED_SPECIAL, 0);  // a.k.a. 2

/// TRUE

const Value V_TRUE = IMMED_V(IMMED_BOOLEAN, 1); // a.k.a. 0x1A

inline int      V_TAG(Value v)      { return ((int) v) & TAG_MASK; }

/// @defgroup preds Type predicates
/// Tests for Value types.
/// @{

/// Is the Value an integer?

inline bool     V_ISINT(Value v)    { return (V_TAG(v) == TAG_INT); }

/// Is the Value a reference to a heap object?

inline bool     V_ISPTR(Value v)    { return (V_TAG(v) & TAG_PTR); }

/// Is the Value a character?

inline bool     V_ISCHAR(Value v)   { return (((int) v) & (TAG_MASK | IMMED_MASK)) == (TAG_IMMED | IMMED_CHAR); }

/// Is the Value a reference to a Binary object?

EXPORT  bool    IsBinary(Value obj);

/// Is the Value a reference to an Array?

EXPORT  bool    IsArray(Value obj);

/// Is the Value a reference to a Frame?

EXPORT  bool    IsFrame(Value obj);

/// Is the Value a reference to a Symbol?

EXPORT  bool    IsSymbol(Value obj);

/// Is the Value a reference to a String?

EXPORT  bool    IsString(Value obj);

/// @}

/// @defgroup conversions Value conversions
/// Functions that convert between C types and Values.
/// @c xxx_V converts an @c xxx to a @c Value.
/// @c V_xxx converts a @c Value to an @c xxx, throwing an exception
/// if the Value is not of that type.
/// @c UNSAFE_V_xxx is like @c V_xxx but does no type checking.
/// (Use only when speed matters and you can prove the type is correct.)
/// @{

/// Converts an int to a Value. Be aware there are only 30 significant bits in an integer Value!

inline Value    INT_V(int i)            { return MAKEVALUE(i, TAG_INT); }

/// Converts a Unicode character to a Value.

inline Value    CHAR_V(short uc)        { return IMMED_V(IMMED_CHAR, uc); }

/// Converts a bool to a Value (false becomes V_NIL, true becomes V_TRUE).

inline Value    BOOL_V(bool b)          { return b ? V_TRUE : V_NIL; }

// Constructs a "magic pointer" Value. Not currently used by anything.

inline Value    MAGICPTR_V(int index)   { return IMMED_V(index, TAG_MAGICPTR); }

/// Converts a double to a Value.

EXPORT Value    REAL_V(double d);

EXPORT int V_INT_error(void);   // Helper functions
EXPORT int V_CHAR_error(void);

/// Converts a Value to an integer with no type check.
/// Quickly converts a Value to an integer *without* testing the type
/// first -- use only if you're sure the Value is an int (or it doesn't much
/// matter if it isn't).

inline int      UNSAFE_V_INT(Value v)   { return (((int) v) >> 2); }

/// Converts a Value to an int. Throws if Value is not an integer.

inline int      V_INT(Value v)          { return V_ISINT(v) ? (((int) v) >> 2) : V_INT_error(); }

/// Converts a Value to a Unicode character. Throws if Value is not a character.

inline int      V_CHAR(Value v)         { return V_ISCHAR(v) ? (((int) v) >> 4) : V_INT_error(); }

/// Converts a Value to a bool. Result is false if Value is V_NIL, otherwise true.

inline bool     V_BOOL(Value v)         { return (v == V_NIL) ? false : true; }

/// Converts a Value to a double. Throws is Value is not a number.

EXPORT double   V_REAL(Value v);

/// @}

/// @defgroup exceptions Exceptions
/// @{

/// Structure used for all Prota exceptions

struct ProtaException {
    const char* name;
    Value   data;

    /// Constructs an exception with given name, error code, and data.
    ProtaException(const char* theName, int err, Value theData);

    /// Constructs an exception with given name and data.
    ProtaException(const char* theName, Value exObj)
        : name(theName), data(exObj)        { }

    /// Constructs an exception with given name and error code.
    ProtaException(const char* theName, int err)
        : name(theName), data(INT_V(err))   { }
};

/// Handy macro to throw a Proto exception. Use whenever possible, in case
/// the exception system changes someday.

#define PROTO_THROW(name, err) throw ProtaException((name), (err))
#define PROTO_THROW_ERR(name, err, data) throw ProtaException((name), (err), (data))

/// Is name1 a subexception of name2?

EXPORT  bool    Subexception(const char* name1, const char* name2);

/// @defgroup exnames Built-in exception names
/// Names of the built-in exceptions
/// @{

const char* const   g_exFr      = "evt.ex.fr";              ///< Base exception name
const char* const   g_exIntrp   = "evt.ex.fr.intrp";        ///< Interpreter errors
const char* const   g_exType    = "evt.ex.fr.type";         ///< Type mismatch errors
const char* const   g_exCompiler = "evt.ex.fr.compiler";    ///< Compiler errors

/// @}
/// @}

/// Call once at the beginning of time to initialize the object system.

EXPORT  void    InitProtoLib(void);

/// Are Values a and b equal? Tests values of immediates, reals and symbols,
/// otherwise tests reference (pointer) equality.

EXPORT  bool    V_EQ(Value a, Value b);

/// @defgroup alloc Object allocation
/// @{

/// Allocates a new Binary object.

EXPORT  Value   NewBinary(Value cls, int size);

/// Allocates a new Binary object with the given data.

EXPORT  Value   NewBinary(Value cls, void* pData, int size);

/// Allocates a new Array object.

EXPORT  Value   NewArray(Value cls, int nSlots);

/// Allocates a new Array object of class @c Array.

EXPORT  Value   NewArray(int nSlots);

/// Allocates a new Frame object.

EXPORT  Value   NewFrame(void);

/// Allocates a new Frame map with the given tags.

EXPORT  Value   NewMapWithTags(Value tagArray);

/// Allocates a new Frame with the given map.

EXPORT  Value   NewFrameWithMap(Value map);

/// @}

/// @defgroup access Access functions
/// Functions for getting and setting data in objects.
/// @{

/// Gets an array slot's value.

EXPORT  Value   GetSlot(Value array, int index);

/// Sets an array slot's value.

EXPORT  void    SetSlot(Value array, int index, Value newValue);

/// Gets a frame slot's value.

EXPORT  Value   GetSlot(Value frame, Value tag);

/// Sets a frame slot's value.

EXPORT  void    SetSlot(Value frame, Value tag, Value newValue);

/// Removes a frame slot.

EXPORT  void    RemoveSlot(Value frame, Value tag);

/// Tests existence of a frame slot.

EXPORT  bool    HasSlot(Value frame, Value tag);

/// Gets a slot value determined by a path expression.

EXPORT  Value   GetPath(Value obj, Value path);

/// Sets the value of a slot determined by a path expression.

EXPORT  void    SetPath(Value obj, Value path, Value newValue);

/// Tests existence of a slot determined by a path expression.

EXPORT  bool    HasPath(Value obj, Value path);

/// Gets a pointer to the beginning of a binary object's data

EXPORT  void*   GetData(Value binary);

/// Gets the value of an object's @em class slot.

EXPORT  Value   GetClassSlot(Value obj);

/// Sets the value of an object's @em class slot.

EXPORT  void    SetClassSlot(Value obj, Value cls);

/// Extends an array by one slot.

EXPORT  void    AddArraySlot(Value array, Value newValue);

/// Gets a pointer to an array's slots.

EXPORT  Value*  GetArraySlots(Value array);

/// Gets the numbers of slots in an array.

EXPORT  int     GetArrayLength(Value array);

/// Sets the number of slots in an array.

EXPORT  void    SetArrayLength(Value array, int nSlots);

/// Gets the number of bytes in a Binary object.

EXPORT  int     GetBinaryLength(Value binary);

/// Sets the number of bytes in a Binary object.

EXPORT  void    SetBinaryLength(Value binary, int size);

/// Returns the length of any object, in bytes for binary objects,
/// in slots for frames and arrays.

EXPORT  int     GetObjLength(Value obj);

/// Clones an object (shallowly).

EXPORT  Value   Clone(Value obj);

/// Clones an object (deeply).

EXPORT  Value   DeepClone(Value obj);

/// Causes all references to @a oldObj to refer to @a newObj instead.

EXPORT  void    ReplaceObject(Value oldObj, Value newObj);

/// @}

/// @defgroup symfuncs Symbol functions
/// Symbol-related functions
/// @{

/// Find the Symbol with the given name.

EXPORT  Value   Intern(const char* name);

#define SYM(name) Intern(#name)

/// Same as @c Intern -- some people like this name better.

inline Value    MakeSymbol(const char* name)    { return Intern(name); }

/// Gets the name of a symbol.
EXPORT  const char* SymbolName(Value sym);

/// @}

/// @defgroup strfuncs String functions
/// String-related functions
/// @{

/// Makes a new String object.

EXPORT  Value   NewString(int size);

/// Makes a new String object.

EXPORT  Value   NewString(const TCHAR* str);

/// Gets the null-terminated C string from a String object.

EXPORT  TCHAR*  GetCString(Value str);

/// Appends to a String object (destructive).

EXPORT  void    StrAppend(Value str1, Value str2);

/// @}

/// Reads a stream file and return the top-level object

EXPORT  Value   ReadStreamFile(const char* filename);

/// @defgroup printing Printing
/// Object printer.
/// @{

/// Prints a Value.

EXPORT  void    PrintValue(Value v);

/// Sets the max depth of printer object traversal.

EXPORT  void    SetMaxPrintDepth(int maxDepth);

/// Prints a Value followed by a newline.

EXPORT  void    PrintValueLn(Value v);

/// @}

#endif //__OBJECTS_H__
