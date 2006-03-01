/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created By Bas Vodde Sunday January 2002
*/

#if P_VISUAL_CPP

/*
	Pretty useless warnings will be turned off.
*/
#pragma warning (disable : 4068)	// Unknown pragma
#pragma warning (disable : 4355)	// this is used in Base Initializer list
#pragma warning (disable : 4786)	// Debug info is truncated. Fine but nothing we can do.
#pragma warning (disable : 4514)	// Unreference inline function is removed, Thanks for letting us know
#pragma warning (disable : 4258)	// for loop syntax has changed, I know...


#pragma warning (disable : 4018)	// signed/unsigned mismatch... don't look at them for now. I hope I know what I'm doing
#pragma warning (disable : 4244)	// truncation of data, should be OK

/*
	BAS TODO: Do we want this?
*/

#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define off_t	_off_t
#include <sys/types.h>

/*
	Maarten likes this defines instead pf using C++ operators.
	Windows does not like it therefore Bas will fix it for Windows :)
	nil = NULL
	not = !
	or  = !!
	and = &&
*/

#define not		!
#define or		||
#define and		&&

typedef _int64	int64;

#elif P_CODEWARRIOR

#include "ansi_prefix.win32.h"
#include <stat.h>
#include <size_t.h>

using std::size_t;

#define NOMINMAX

typedef long long	int64;

#endif

#define nil		NULL

// BAS TODO: Please review this hack to get WM_MOUSEWHEEL defined
#define _WIN32_WINNT 0x0501
 
/*
	NATIVE CONTROLES
	BAS TODO: native
*/
#define NATIVE_CONTROLS 1
#define ISOLATION_AWARE_ENABLED	1

//#include "HWinWindows.h"
