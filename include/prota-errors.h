/*
    Prota language runtime

    Error codes

    Copyright 1997-1999 Walter R. Smith
    Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __PROTA_ERRORS_H__
#define __PROTA_ERRORS_H__

// evt.ex.fr

#define E_BadStreamFormat -48006 // Stream file has bad or unknown format
#define E_NotAPointer -48200 // Expected a frame, array, or binary object
#define E_BadMagicPtr -48201 // Invalid magic pointer
#define E_EmptyPath -48202 // Empty path
#define E_InvalidPath -48203 // Invalid segment in path expression
#define E_PathFailed -48204 // Path failed
#define E_OutOfBounds -48205 // Index out of bounds (string or array)
#define E_SameObject -48206 // Source and destination must be different objects
#define E_LongOutOfRange -48207 // Long out of range
#define E_BadArguments -48210 // Bad arguments
#define E_StringTooBig -48211 // String too big
// -48212 Expected a frame, array, or binary object
// -48213 Expected a frame, array, or binary object
#define E_ReadOnly -48214 // Object is read-only
#define E_OutOfMemory -48216 // Out of heap memory
// -48217 Invalid attempted use of magic pointer
#define E_NegativeSize -48218 // Cannot create or change an object to negative size
#define E_ValueOutOfRange -48219 // Value out of range
#define E_LockedObject -48220 // Could not resize locked object
// -48221 Reference to deactivated package
#define E_NotEvtEx -48222 // Exception is not a subexception of |evt.ex|
#define E_Internal -48223 // Aborted due to internal object system inconsistency

// evt.ex.fr.type
#define E_NotAFrame -48400 // Expected a frame
#define E_NotAnArray -48401 // Expected an array
#define E_NotAString -48402 // Expected a string
// -48403 // Expected a frame, array, or binary object
#define E_NotANumber -48404 // Expected a number
#define E_NotAReal -48405 // Expected a real
#define E_NotAnInteger -48406 // Expected an integer
#define E_NotACharacter -48407 // Expected a character
#define E_NotABinary -48408 // Expected a binary object
#define E_NotAPath -48409 // Expected a path expression (or a symbol or integer)
#define E_NotASymbol -48410 // Expected a symbol
#define E_NotAFunction -48411 // Expected a function
#define E_NotAFrameOrArray -48412 // Expected a frame or an array
#define E_NotAnArrayOrNIL -48413 // Expected an array or nil
#define E_NotAStringOrNIL -48414 // Expected a string or nil
#define E_NotABinaryOrNIL -48415 // Expected a binary object or nil
#define E_UnexpectedFrame -48416 // Unexpected frame
#define E_UnexpectedBinary -48417 // Unexpected binary object
#define E_UnexpectedImmediate -48418 // Unexpected immediate
#define E_NotAnArrayOrString -48419 // Expected an array or string
// -48420 Expected a virtual binary object
// -48421 Expected a package
#define E_NotNIL -48422 // Expected nil
#define E_NotASymbolOrNIL -48423 // Expected nil or a symbol
#define E_NotTRUEOrNIL -48424 // Expected nil or true
#define E_NotAnIntegerOrArray -48425 // Expected an integer or an array

// evt.ex.fr.intrp
#define E_NotInBreakLoop -48800 // Not in a break loop
#define E_WrongNumArgs -48803 // Wrong number of arguments
#define E_ZeroForLoopIncr -48804 // FOR loop BY expression has value zero
#define E_NoCurrentException -48806 // No current exception
#define E_UndefinedVariable -48807 // Undefined variable
#define E_UndefinedFunction -48808 // Undefined global function
#define E_UndefinedMethod -48809 // Undefined method
#define E_NoProto -48810 // No _proto for inherited send
#define E_NILSlotAccess -48811 // Tried to access slot of nil
#define E_InvalidBytecode - 48812 // Invalid bytecode

// evt.ex.fr.compiler
#define E_SyntaxError -49000 // Syntax error

#endif //__PROTA_ERRORS_H__
