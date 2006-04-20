/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Thursday October 04 2001 12:43:52
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

#include <sys/time.h>
#include <unistd.h>
//#include <pwd.h>
#include "HStdCStdlib.h"
#include "HStlString.h"
#include "HStdCStdarg.h"
#include "HStdCStdio.h"

#include "HError.h"
#include "HUtils.h"
#include "HGlobals.h"
#include "HXGlobals.h"
//#include "HPreferences.h"
#include "HFile.h"

HProfile::HProfile()
{
}

HProfile::~HProfile()
{
}

double system_time()
{
	struct timeval tv;
	
	gettimeofday(&tv, nil);
	
	return tv.tv_sec + tv.tv_usec / 1e6;
}

double get_dbl_time()
{
	return 0.5;
}

double get_caret_time()
{
	return 0.5333;
}

void delay(double inSeconds)
{
	if (inSeconds > 0)
	{
		unsigned long uSecs = static_cast<unsigned long>(inSeconds * 1000000);
		::usleep(uSecs);
	}
}

/*bool GetUserName(std::string& outName)
{
	struct passwd* pw = getpwuid(gUid);
	if (pw)
	{
		outName = pw->pw_gecos;
		
		if (outName.length() > 0)
		{
			std::string::size_type p = outName.find(',');
			if (p != std::string::npos)
				outName.erase(p, outName.length() - p);
			p = outName.find('&');
			if (p != std::string::npos)
				outName.replace(p, 1, pw->pw_name);
		}
		else
			outName = pw->pw_name;
	}
	return pw != nil && outName.length() > 0;
}
*/
void debug_printf(const char* msg, ...)
{
	using namespace std;

	va_list vl;
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
}

void set_debug_info (const char* inFilename, int inLineNumber)
{
}

/*
bool UsePassiveFTPMode()
{
	const char* e = getenv("FTP_PASSIVE_MODE");
	return e != nil && std::strcasecmp(e, "YES") == 0;
}
*/

std::string GetFormattedFileDateAndTime(int64)
{
	return "";
}

bool GetFormattedBool()
{
	return false;
}
