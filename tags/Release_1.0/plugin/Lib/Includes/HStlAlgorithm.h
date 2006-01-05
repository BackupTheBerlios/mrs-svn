/*	$Id: HStlAlgorithm.h,v 1.13 2005/08/22 12:38:05 maarten Exp $
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
 
#ifndef HSTLALGORITHM_H
#define HSTLALGORITHM_H

/*
	HStlAlgorithm just includes the platform specific algorithm (when
	its available). It can solve platform specific STL problems.

	Also might give you the chance to switch stl version when needed.
*/

#if P_VISUAL_CPP
	#pragma warning (push)
	#pragma warning (disable : 4100)
	#pragma warning (disable : 4290)
	#pragma warning (disable : 4512)
#endif

#include <algorithm>

#if P_VISUAL_CPP
	#pragma warning (pop)
#endif

#if P_VISUAL_CPP

namespace std 
{
	template <class T>
	T min (T val1, T val2)
	{
		return (val1 < val2) ? val1 : val2;
	}
	template <class T>
	T max (T val1, T val2)
	{
		return (val2 < val1) ? val1 : val2;
	}

/* 
	Visual C++ does something with abs in Release mode.
	If the function is not called abs then there is no problem
*/
#ifdef P_VISUAL_CPP
	#define abs visual_sucks_abs
#endif

    template <class T>
    T abs(T val)
    {
        return val < 0 ? -val : val;
    }
};

#elif P_GNU

namespace std
{
    template <class T>
    T abs(T val)
    {
        return val < 0 ? -val : val;
    }
}

#endif

#endif // HSTLALGORITHM_H
