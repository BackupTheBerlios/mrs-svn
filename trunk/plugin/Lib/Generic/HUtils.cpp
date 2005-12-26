/*	$Id: HUtils.cpp,v 1.61 2005/08/22 12:38:04 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	M.L. Hekkelman
	
	Created: 03 March, 2000
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

#include "HStdCCtype.h"
#include "HStlStack.h"
#include "HStdCFcntl.h"
#include "HStdCString.h"
#include "HStdCStdlib.h"
#include "HStdCTime.h"
#include "HStdCStdio.h"
#include "HStdCMath.h"

#include "HUtils.h"
#include "HFile.h"
#include "HError.h"
//#include "HPreferences.h"
#include "HGlobals.h"

using namespace std;

void EscapeString(string& ioString, bool inEscape)
{
	unsigned long indx;

	if (inEscape)
	{
		indx = 0;
		while ((indx = ioString.find("\\", indx)) != string::npos)
		{
			ioString.replace(indx, 1, "\\\\");
			indx += 2;
		}

		indx = 0;
		while ((indx = ioString.find("\t", indx)) != string::npos)
			ioString.replace(indx++, 1, "\\t");
		
		indx = 0;
		while ((indx = ioString.find("\r", indx)) != string::npos)
			ioString.replace(indx++, 1, "\\r");
		
		indx = 0;
		while ((indx = ioString.find("\n", indx)) != string::npos)
			ioString.replace(indx++, 1, "\\n");
	}
	else
	{
		indx = 0;
		while (indx + 1 < ioString.length())
		{
			if (ioString[indx++] == '\\')
			{
				char ch = ioString[indx];
				switch (ch)
				{
					case 'n':
						ioString[indx - 1] = '\n';
						ioString.erase(indx--, 1);
						break;
					case 'r':
						ioString[indx - 1] = '\r';
						ioString.erase(indx--, 1);
						break;
					case 't':
						ioString[indx - 1] = '\t';
						ioString.erase(indx--, 1);
						break;
					case '\\':
						ioString[indx - 1] = '\\';
						ioString.erase(indx--, 1);
						break;
					default:
						ioString.erase(--indx, 1);
						break;
				}
				++indx;
			}
		}
	}
}

#if TARGET_API_MAC_CARBON
char* p2cstr(unsigned char* inString)
{
	unsigned long l = inString[0];
	memmove(inString, inString + 1, l);
	inString[l] = 0;
	return reinterpret_cast<char*>(inString);
}

unsigned char* c2pstr(char* inString)
{
	unsigned long l = strlen(inString);
	if (l > 255) l = 255;
	memmove(inString + 1, inString, l);
	unsigned char* result = reinterpret_cast<unsigned char*>(inString);
	result[0] = static_cast<unsigned char>(l);
	return result;
}
#else
void c2pstrcpy(unsigned char *p, const char *s)
{
	p[0] = 0;
	
	char *t = (char *)p + 1;
	
	while (*s)
		*t++ = *s++, ++(p[0]);
}

void p2cstrcpy(char *p, const unsigned char *s)
{
	int l = *s++;
	
	while (l--)
		*p++ = static_cast<char>(*s++);
	
	*p = 0;
}
#endif

char* 
string_copy (const char* inString)
{
	char* result = NULL;
	if (inString)
	{
		unsigned long l = strlen (inString);
		result = new char [l + 1];
		if (result) {
			memcpy(result, inString, l + 1);
		}
	}
	return result;
}

string NumToString(int inNumber)
{
	char s[32];
	snprintf(s, 32, "%d", inNumber);
	return s;
}

void NormalizePath(string& ioPath)
{
	string path(ioPath);	
	stack<unsigned long> dirs;
	int r = 0;
	unsigned long i = 0;
#if P_WIN
	// on windows a double slash is significant when the path start with it
	bool isNetworkDrive = false;
	if (ioPath.length() > 3 && ioPath[0] == '/' && ioPath[1] == '/' && ioPath[2] == '/')
		isNetworkDrive = true;
#endif
	
	dirs.push(0);
	
	while (i < path.length())
	{
		while (i < path.length() && path[i] == '/')
		{
			++i;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
		}
		
		if (path[i] == '.' && path[i + 1] == '.' && path[i + 2] == '/')
		{
			if (dirs.size() > 0)
				dirs.pop();
			if (dirs.size() == 0)
				--r;
			i += 2;
			continue;
		}
		else if (path[i] == '.' && path[i + 1] == '/')
		{
			i += 1;
			if (dirs.size() > 0)
				dirs.top() = i;
			else
				dirs.push(i);
			continue;
		}

		unsigned long d = path.find('/', i + 1);

		if (d == string::npos)
			break;
		
		i = d + 1;
		dirs.push(i);
	}
	
	if (dirs.size() > 0 && dirs.top() == path.length())
		ioPath.assign("/");
	else
		ioPath.erase(ioPath.begin(), ioPath.end());
	
	bool dir = false;
	while (dirs.size() > 0)
	{
		unsigned long l, n;
		n = path.find('/', dirs.top());
		if (n == string::npos)
			l = path.length() - dirs.top();
		else
			l = n - dirs.top();
		
		if (l > 0)
		{
			if (dir)
				ioPath.insert(0, "/");
			ioPath.insert(0, path.c_str() + dirs.top(), l);
			dir = true;
		}
		
		dirs.pop();
	}
	
	if (r < 0)
	{
		ioPath.insert(0, "../");
		while (++r < 0)
			ioPath.insert(0, "../");
	}
	else if (path.length() > 0 && path[0] == '/' && ioPath[0] != '/')
		ioPath.insert(0, "/");

#if P_WIN
	if (isNetworkDrive)
		ioPath.insert(0, "//");
#endif
}

#pragma mark -

HTextWriter::HTextWriter(const HUrl& inUrl)
	: fFD(HFile::Open(inUrl, O_RDWR | O_CREAT | O_TRUNC))
{
	ThrowIfPOSIXErr(fFD);
}

HTextWriter::~HTextWriter()
{
	HFile::Close(fFD);
}

HTextWriter& HTextWriter::operator << (const char *inString)
{
	ThrowIfPOSIXErr(HFile::Write(fFD, inString, strlen(inString)));
	return *this;
}

HTextWriter& HTextWriter::operator << (string inString)
{
	ThrowIfPOSIXErr(HFile::Write(fFD, inString.c_str(), inString.length()));
	return *this;
}

void DoubleToTM(double inTime, struct tm& outTM)
{
	time_t t = static_cast<time_t>(inTime);
	outTM = *localtime(&t);
}

void TMToDouble(const struct tm& inTM, double& outDouble)
{
	outDouble = mktime(const_cast<struct tm*>(&inTM));
}

unsigned short CalculateCRC(const void* ptr, long count, unsigned short crc)
{
	unsigned short	i;
	const unsigned char* p = reinterpret_cast<const unsigned char*>(ptr);

	while (count -- > 0) {
		crc = (unsigned short) (crc ^ (unsigned short)*p++ << 8);
		for (i = 0; i < 8; i++)
			if (crc & 0x8000)
				crc = (unsigned short) ((crc << 1) ^ 0x1021);
			else
				crc = (unsigned short) (crc << 1);
	}

	return crc;
}

bool Code1Tester(const char* inName, unsigned int inSeed, unsigned int inCode)
{
	unsigned short crc = CalculateCRC(inName, static_cast<long>(strlen(inName)),
		static_cast<unsigned short>(inSeed));
	return crc == static_cast<unsigned short>(inCode);
}

string GetFormattedDate()
{
	char b[256];

	time_t t;
	time(&t);
	strftime(b, 256, "%x", localtime(&t));

	return b;
}

string GetFormattedTime()
{
	char b[256];

	time_t t;
	time(&t);
	strftime(b, 256, "%X", localtime(&t));

	return b;
}

string GetFormattedFileDateAndTime(double inDateTime)
{
	const char* format = "%d-%b-%Y";
	
	struct tm tm;
	::DoubleToTM(inDateTime, tm);

	char b[256];

	strftime(b, 256, format, &tm);
	return b;
}

void copy_to_charbuf(char* inString, const string& inText, int inStringSize)
{
	int n = inText.length();
	if (n > inStringSize - 1)
		n = inStringSize - 1;
	inText.copy(inString, n);
	inString[n] = 0;
}

void FormatHex(HUnicode inUnicode, char* outString, int inBufferSize)
{
	const char kPrefix[] = "%";
	const uint32 kPrefixLength = sizeof(kPrefix) - 1;
	
	assert(inBufferSize >= 12);

	char* c;

	if (inUnicode > 0x00ffffUL)
		c = outString + 8 + kPrefixLength;
	else if (inUnicode > 0x00ffUL)
		c = outString + 4 + kPrefixLength;
	else
		c = outString + 2 + kPrefixLength;
	
	*c-- = 0;
	
	while (inUnicode && c >= outString + kPrefixLength)
	{
		*c-- = kHexChars[inUnicode & 0x0f];
		inUnicode >>= 4;
	}
	
	while (c >= outString + kPrefixLength)
		*c-- = '0';
	
	while (c >= outString)
	{
		*c = kPrefix[c - outString];
		--c;
	}
}


