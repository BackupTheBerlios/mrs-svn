/*	$Id: HLib.h 331 2007-02-12 07:44:10Z hekkel $
	
	Copyright Hekkelman Programmatuur b.v.
	Bas Vodde
	
	Created: 01/13/01 23:02:18
*/

/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
/*
	This file is the "general config" file. It will determine what OS
	is running and setting a few very very basic defines. These will be 
	used later in the platform independend code.

*/

#ifndef HLIB_H
#define HLIB_H

/*
	Determine the destination compiler. In general all the compiler have a 
	compiler independend wat to determine if its "theirs". Use this and set
	the P_compiler to the right compiler.
*/

/*
	Microsoft Visual C++
*/
#if defined(_MSC_VER) && !defined(__MWERKS__)
	#if (_MSC_VER >= 1200)
		#define P_VISUAL_CPP	1
	#else
		#error "You need VC6 or higher"
	#endif
#else
	#define P_VISUAL_CPP		0
#endif

/*
	Borland
*/
#if defined(__BORLANDC__)
	#define P_BORLAND			1
#else
	#define	P_BORLAND			0
#endif

/*
	Code warrior
*/
#ifdef __MWERKS__
	#define P_CODEWARRIOR		1
#else
	#define P_CODEWARRIOR		0
#endif

/*
	GCC
*/
#ifdef __GNUC__
	#define P_GNU				1
#else
	#define P_GNU				0
#endif

/*
	MIPSpro, must be able to do this better, have to find out
*/
#if defined(IRIX) && ! defined(__GNUC__)
	#define P_MIPSPRO			1
#endif

/*
	Check if we are using a known compiler. If not it needs to be added!
*/
#if ((P_VISUAL_CPP + P_CODEWARRIOR + P_GNU + P_BORLAND + P_MIPSPRO) != 1)
		#error "Using an unknown compiler."
#endif

/*
	Now use the compiler to determine what platform the compiler is running
	on. For every platform there will be several defines. The P_DEST_OS will
	be containing an integer with the current platform. 
*/
#define P_MAC_OS			1
#define P_UNIX_OS			2
#define P_WIN32_OS			3
#define P_BE_OS				4

/*
	Codewarrior works mainly for Mac and Win. Fix for the linux version
*/
#if P_CODEWARRIOR
	#if macintosh
		#define P_DEST_OS	P_MAC_OS
		#define P_MAC		1
	#elif __POWERPC__
		#define P_DEST_OS	P_UNIX_OS
		#define P_UNIX		1
		#define P_MACOSX	1
	#else
		#define P_DEST_OS	P_WIN32_OS
		#define P_WIN		1 
	#endif
#endif

/*
	Visual C++ and Borland work only for Windows
*/
#if P_VISUAL_CPP || P_BORLAND
	#define P_DEST_OS		P_WIN32_OS
	#define P_WIN			1
#endif

#if P_MIPSPRO
	#define P_DEST_OS		P_UNIX_OS
	#define P_UNIX			1
	#define P_IRIX			1
#endif

/*
	Gnu works for more than just unix. For now just
	define it as a unix compiler.
*/
#if P_GNU
    #if defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__)
    #	define P_DEST_OS		P_MAC_OS
	#	define P_UNIX			1
	#	define P_DARWIN			1
    #elif DARWIN
    #	define P_DEST_OS		P_UNIX_OS
    #	define P_UNIX			1
    #	define P_DARWIN			1
    #elif FREEBSD
	#	define P_DEST_OS		P_UNIX_OS
	#	define P_UNIX			1
	#	define P_FREEBSD		1
    #elif LINUX
	#	define P_DEST_OS		P_UNIX_OS
	#	define P_UNIX			1
	#	define P_LINUX			1
	#elif IRIX
	#	define P_DEST_OS		P_UNIX_OS
	#	define P_UNIX			1
	#	define P_IRIX			1
	#elif WINDOWS
	#	define P_DEST_OS		P_WIN32_OS
	#	define P_WIN			1
    #else
	#	define P_DEST_OS		P_UNIX_OS
	#	define P_UNIX			1
	#error
    #endif
#endif

/*
	Make sure that the P_xxx are defines correctly. That is... on OR off
*/
#ifndef P_MAC
	#define P_MAC			0
#endif
#ifndef P_DARWIN
	#define P_DARWIN		0
#endif
#ifndef P_WIN
	#define P_WIN			0
#endif
#ifndef P_UNIX
	#define P_UNIX			0
#endif
#ifndef P_BE
#	define P_BE				0
#endif
#ifndef P_FREEBSD
#	define P_FREEBSD		0
#endif
#ifndef P_LINUX
#	define P_LINUX			0
#endif
#ifndef P_IRIX
#	define P_IRIX			0
#endif
#ifndef P_MACOSX
#	define P_MACOSX			0
#endif

/*
	Check if we are compiling on a known OS!
*/
#if ((P_DEST_OS != P_MAC_OS) && (P_DEST_OS != P_UNIX_OS) && (P_DEST_OS != P_WIN32_OS))
	#error "Using an unknown platform"
#endif
#if (P_WIN + P_MAC + P_UNIX) != 1
	#error "Using an unknown platform"
#endif
#if P_UNIX && (P_LINUX + P_FREEBSD + P_IRIX + P_MACOSX + P_DARWIN) != 1
	#error "Should have only one flavor of unix"
#endif

/*
	Determine what CPU we are compiling on!
	The CPU will be stored in P_DEST_CPU
*/

#define P_CPU_INTEL			1
#define P_CPU_POWERPC		2
#define P_CPU_MIPS			3

#if P_GNU
#	if defined(__ppc__)
#		define P_DEST_CPU	P_CPU_POWERPC
#		define P_POWERPC	1
#	elif defined(__i386__) || defined(__amd64__) || defined(CPU_x86)
#		define P_DEST_CPU	P_CPU_INTEL
#		define P_INTEL		1
#	else
#		error
#	endif
#elif P_CODEWARRIOR
#	define P_DEST_CPU		P_CPU_POWERPC
#	define P_POWERPC		1
#elif P_UNIX && !defined(P_DEST_CPU)
#	if P_IRIX
#		define P_DEST_CPU	P_CPU_MIPS
#		define P_MIPS		1
#	elif defined(CPU_ppc)
#		define P_DEST_CPU	P_CPU_POWERPC
#		define P_POWERPC	1
#	elif defined(CPU_x86)
#		define P_DEST_CPU	P_CPU_INTEL
#		define P_INTEL		1
#	else
#		error "Define processor type in makefile please"
#	endif
#endif

#if P_WIN
	#define P_DEST_CPU		P_CPU_INTEL
	#define P_INTEL			1
#endif

#ifndef P_POWERPC
	#define P_POWERPC		0
#endif
#ifndef P_INTEL
	#define P_INTEL			0
#endif

/*
	Checking if its done right
*/
#if ((P_DEST_CPU != P_CPU_INTEL) && (P_DEST_CPU != P_CPU_POWERPC) && (P_DEST_CPU != P_CPU_MIPS))
	#error "Compiling on unknown CPU"
#endif
#if (P_INTEL + P_POWERPC + P_MIPS) != 1
	#error "Compiling on unknown CPU"
#endif

/*
	Some very basic types needed for some OS.
*/

#if P_UNIX && P_GNU && !P_IRIX
	#define _GNU_SOURCE		1
	#if ! P_MAC && ! P_FREEBSD && !P_DARWIN
		#include <features.h>
	#endif
#endif

/*
	Define the endianness. This can be done by CPU.
	Use the P_DEST_ENDIAN for this.
*/

#define P_ENDIAN_BIG		0
#define P_ENDIAN_LITTLE		1

#if P_UNIX && ! P_FREEBSD && ! P_IRIX && !P_DARWIN
	#if P_MACOSX
	#	include <machine/endian.h>
	#else
	#	include <endian.h>
	#endif
#endif

#if P_INTEL
	#define P_DEST_ENDIAN	P_ENDIAN_LITTLE
	#define P_LITTLEENDIAN	1
#endif

#if P_POWERPC || P_MIPS
	#define P_DEST_ENDIAN	P_ENDIAN_BIG
	#define P_BIGENDIAN		1
#endif

#ifndef P_BIGENDIAN
	#define P_BIGENDIAN		0
#endif
#ifndef P_LITTLEENDIAN
	#define P_LITTLEENDIAN	0
#endif

#if ((P_DEST_ENDIAN != P_ENDIAN_BIG) && (P_DEST_ENDIAN != P_ENDIAN_LITTLE))
	#error "Target endian unknown"
#endif
#if (P_BIGENDIAN + P_LITTLEENDIAN) != 1
	#error "Target endian unknown"
#endif

/*
	NULL define. Sometimes the stdC is not used but NULL is
	Stroustrup says: in C++ just 0 should be the best NULL.
*/
#ifndef NULL
#define NULL	0
#endif

// This is for pcre, the perl compatible regular expressions lib.
#define SUPPORT_UTF8					1

/*
	Include a native file to do some extra platform specific things.
	We could have moved all the previous work in there too but IMHO
	its better to keep that together.
	
	OK, but make sure you define what is needed for the platform, see
	checks below. (MLH)
*/

#include "HLibImp.h"

// Some post imp checks

#ifndef NATIVE_CONTROLS
#	define NATIVE_CONTROLS		0
#endif

#ifndef THEME_CONTROLS
#	define THEME_CONTROLS		0
#endif

#if (! NATIVE_CONTROLS && ! THEME_CONTROLS)
	#error "Either native or theme controls are needed"
#endif

/*
	Set the DEBUG symbollic constant. For our own library we will
	use P_DEBUG. We make sure that DEBUG and _DEBUG are also
	defined!
*/

#if !defined(NDEBUG) && !defined(DEBUG)
#define	DEBUG			1
#endif

#if DEBUG
	#define P_DEBUG		1
	#undef DEBUG
#endif
#if _DEBUG
	#define P_DEBUG		1
	#undef _DEBUG
#endif

#if P_DEBUG
	#define DEBUG		1
	#define _DEBUG		1
	#define P_NDEBUG	0
	#undef NDEBUG
	#undef _NDEBUG
#else
	#define P_DEBUG		0
	#define DEBUG		0
	#define _DEBUG		0
	#define _NDEBUG		1
	#define NDEBUG		1
	#define P_NDEBUG	1
#endif

/*
	NULL variable can be used to get rid of "constant conditional" warning. On
	debug it will point to a global variable which is always 0. In release
	mode it will be just (0).
*/
#if P_DEBUG
	extern int gGlobalFalse;
	#define P_NULL_VAR	gGlobalFalse
//
//#	define _MEMDEBUG
//#	include <cstdlib>
//#	include "memtoolscpp.h"

#else
	#define P_NULL_VAR	0
#endif

/*
	Some very basic #includes that should not harm anything!
	These includes SHOULD be VERY small and not include all 
	stdC or stl (does not degrade compilation speed)
	BAS TODO: Maybe remove... reconsider
*/

#if P_UNIX && ! P_IRIX
#	define _ASSERT_H
#	if P_DEBUG

#include <cstdio>

#undef __builtin_expect
#define __builtin_expect(x,y)	(x)

		extern "C" void __assertion_failed(const char*, const char*, int);
#		define assert(x)	do { if (not (x)) __assertion_failed(#x, __FILE__, __LINE__); } while (false)
#		define _ASSERT_H
#	else
#		define assert(x)
#	endif
#else
#	include <cassert>
#endif

#ifndef P_PROFILE
#	define P_PROFILE 0
#endif

// Cannot believe it but it is really true:
#ifndef NULL
#define NULL	0
#endif

typedef char				int8;
typedef unsigned char		uint8;
typedef short				int16;
typedef unsigned short		uint16;

typedef int					int32;
typedef unsigned int		uint32;

#if P_BORLAND
typedef __int64				int64;
typedef unsigned __int64	uint64;
#else
typedef long long			int64;
typedef unsigned long long	uint64;
#endif

#endif
