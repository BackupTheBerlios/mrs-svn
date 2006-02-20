/*	$Id$
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
 
#ifndef HSTDMATH_H
#define HSTDMATH_H

/*
	HStdCMath just includes the platform specific std c math (when
	its available). It can solve platform specific STD c problems.

	Windows:	the std namespace in stl is not always 'done'. Therefore
				add inline helper functions in the std namespace.
*/

#if IRIX
#include <math.h>
#else
#include <cmath>
#endif

#if P_VISUAL_CPP

namespace std
{
//	using ::abs;

	template<typename T>
	T trunc(T a)
	{
		T result;

		if (a < 0)
			result = ceil(a);
		else
			result = floor(a);

		return result;
	}
}

#endif

#if P_VISUAL_CPP || P_GNU

namespace std
{
	inline
	long roundtol (double d)
	{
		if (d > 0.0) {
			return long (d + 0.5);
		}
		else {
			return long (d - 0.5);
		}
	}
	using ::fmod;
}

#elif P_CODEWARRIOR && P_MAC

#include <fp.h>

//namespace std
//{
//	inline long roundtol(double d)
//	{
//		return ::roundtol(d);
//	}
//}

#endif

#endif // HSTDSTRING_H
