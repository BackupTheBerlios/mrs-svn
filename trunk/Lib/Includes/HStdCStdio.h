/*	$Id: HStdCStdio.h,v 1.25 2005/08/22 12:38:05 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Wednesday January 16 2002 15:01:20
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
 
#ifndef HSTDCSTDIO_H
#define HSTDCSTDIO_H

/*
	HStdCStdio just includes the platform specific std c stdio (when
	its available). It can solve platform specific STD c problems.

*/

#if IRIX
#include <stdio.h>
#include <stdarg.h>
#else
#include <cstdio>
#include <cstdarg>
#endif

#if P_IRIX

namespace std
{
//	using ::size_t;
//	using ::printf;
//	using ::fflush;
	using ::snprintf;
	using ::vsnprintf;
//	using ::sscanf;
}

#elif P_VISUAL_CPP

namespace std
{
	using ::size_t;
	using ::printf;
	using ::fflush;
	using ::snprintf;
	using ::snprintf;
	using ::sscanf;
	using ::_snprintf;
	using ::_vsnprintf;
}

#elif P_GNU

#if __GNUC__ < 3

namespace std
{
	using _STLP_VENDOR_CSTD::snprintf;
	using _STLP_VENDOR_CSTD::snprintf;
	using _STLP_VENDOR_CSTD::vsnprintf;
	using _STLP_VENDOR_CSTD::vfprintf;
	using _STLP_VENDOR_CSTD::fprintf;
}

#else

namespace std
{
	using ::snprintf;
	using ::vsnprintf;
}

#endif

//#elif P_CODEWARRIOR
//
//using std::FILE;

#endif

#endif // HSTDCSTDIO_H
