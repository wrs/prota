/*
	Proto language runtime

	Definitions for using compile-time objects

	Copyright 1997-1999 Walter R. Smith
	Licensed under the MIT License. See LICENSE file in project root.
*/

#ifndef __PREDEFINED_H__
#define __PREDEFINED_H__

#define PREDEF_NAME(name)	predef_##name
#define V_PTRTO(name)		((Value) (((int) &name) + 1))
#ifdef NSRT_BUILD
 #define DLLSPEC __declspec(dllexport)
#else
 #define DLLSPEC __declspec(dllimport)
#endif
#define DECLARE_POBJ(name)	extern "C" int DLLSPEC PREDEF_NAME(name)
#define POBJ(name)			V_PTRTO(PREDEF_NAME(name))
#define PREDEF_SYM_NAME(name)	sym_##name
#define DECLARE_PSYM(name)	DECLARE_POBJ(PREDEF_SYM_NAME(name))
#define PSYM(name)			POBJ(PREDEF_SYM_NAME(name))

#endif //__PREDEFINED_H__
