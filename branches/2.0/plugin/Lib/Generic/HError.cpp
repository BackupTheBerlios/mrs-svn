/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Thursday September 06 2001 12:24:15
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
 
#include "HLib.h"

#include <typeinfo>

#include "HError.h"
#include "HGlobals.h"
#include "HUtils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>

#include <iostream>

using namespace std;

#if __MWERKS__ < 0x3000
#define ASSERTION_FAILED	__assertion_failed
#else
//#error
#define ASSERTION_FAILED	__msl_assertion_failed
#endif

#if P_DEBUG
bool gOKToThrow = false;
bool gLoopFalse = false;

#if P_MAC && ! P_GNU
const char __func__[] = "unknown";
#endif

#endif

HError::HError(const char* inMessage, ...)
{
	using namespace std;
	
	va_list vl;
	va_start(vl, inMessage);
	vsnprintf(fMessage, 256, inMessage, vl);
	va_end(vl);
#if P_DEBUG
	cerr << fMessage << endl;
#endif
}

const char* HError::what() const throw()
{
	return fMessage;
}

void DisplayError(const exception& inErr)
{
	if (typeid(inErr) == typeid(HError))
		cerr << "Exception: " << static_cast<const HError&>(inErr).GetErrorString() << endl;
	else
		cerr << "Exception: " <<  inErr.what() << endl;
}

#if P_DEBUG
void ReportThrow(const char* inFunc, const char* inFile, int inLine)
{
	if (gOKToThrow)
		return;
	
	cerr << endl << "Exception in " << inFunc << ", " << inFile << ':' << inLine << endl;
}

void ASSERTION_FAILED(char const *inErr, char const *inFile, int inLine)
{
	cerr << endl << "Assertion failure (" << inErr << ") in " << inFile << ':' << inLine << endl;
	abort();
}
#endif

#if P_DEBUG
StOKToThrow::StOKToThrow()
	: fWasOK(true)
{
	swap(fWasOK, gOKToThrow);
}

StOKToThrow::~StOKToThrow()
{
	swap(fWasOK, gOKToThrow);
}

#endif
