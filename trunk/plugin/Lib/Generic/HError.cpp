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

#if MINI_H_LIB
#include <iostream>
#else
//#include "HAlerts.h"
//#include "HStrings.h"
#endif

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

#ifndef MINI_H_LIB
HError::HError(HErrorCode inErr, const char* inMsg)
	: fErrCode(inErr)
	, fInfo(0)
{
	string msg = HStrings::GetFormattedErrString(inErr, inMsg ? string(inMsg) : kEmptyString);
	if (msg.length())
		copy_to_charbuf(fMessage, msg, sizeof(fMessage));
	else
		strncpy(fMessage, inMsg, 256);
}

HError::HError(HErrorCode inErr, int inInfo)
	: fErrCode(inErr)
	, fInfo(inInfo)
{
/*#if ! P_WIN
	if (fErrCode == pErrPosix)
		snprintf(fMessage, 256, HStrings::GetErrString(pErrPosix).c_str(),
			strerror(inInfo));
	else
	{
#endif
*/
		string msg = HStrings::GetFormattedErrString(inErr, NumToString(inInfo));
		if (msg.length())
			copy_to_charbuf(fMessage, msg, sizeof(fMessage));
		else
			snprintf(fMessage, 256, "Unknown error %d", inInfo);
//#if ! P_WIN
//	}
//#endif
}
#endif

const char* HError::what() const throw()
{
	return fMessage;
}

#if MINI_H_LIB
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

#else
void DisplayError(const exception& inErr)
{
	try
	{
		if (typeid(inErr) == typeid(HError))
			HAlerts::Alert(nil, 205, static_cast<const HError&>(inErr).GetErrorString());
		else
			HAlerts::Alert(nil, 205, inErr.what());
	}
	catch (...) {}
}

#if P_DEBUG
void ReportThrow(const char* inFunc, const char* inFile, int inLine)
{
#if P_UNIX
PRINT(("exception in %s, %s, %d\n", inFunc, inFile, inLine));
#endif

	if (gOKToThrow)
		return;
	
	static bool sInReport = false;
	
	if (sInReport)
		return;
	
	HValueChanger<bool> save(sInReport, true);
	
	try
	{
		switch (HAlerts::Alert(nil, 500, inFunc, inFile, NumToString(inLine)))
		{
			case 2:
				abort();
				break;
			case 3:
//				::Debugger();
				int a = 1, b = 0, c = a / b;
				break;
		}
	}
	catch (...) {}
}

void ASSERTION_FAILED(char const *inErr, char const *inFile, int inLine)
{
#if P_UNIX
	PRINT(("assertion failed: '%s' at %s, line %d\n", inErr, inFile, inLine));
#endif
	
	static bool sAlreadyOnScreen = false;
	if (sAlreadyOnScreen)
		return;
	HValueChanger<bool> save(sAlreadyOnScreen, true);
	
	try
	{
		switch (HAlerts::Alert(nil, 501, inErr, inFile, NumToString(inLine)))
		{
			case 2:
				abort();
				break;
			case 3:
#if P_MAC
				::Debugger();
#elif P_WIN
				long l = 0, n = 1;
				n /= l;
#endif
				break;
		}
	}
	catch (...) {}
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

#endif
