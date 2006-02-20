/*	$Id$
	Copyright hekkelman
	Created Wednesday January 18 2006 13:40:33
*/

#ifndef HENCODING_H
#define HENCODING_H

#include "HStlString.h"

#if P_WIN
typedef std::wstring				UTF16String;
#else
typedef std::basic_string<uint16>	UTF16String;
#endif

typedef std::string					UTF8String;

UTF16String Convert(const UTF8String& inText);
UTF8String Convert(const UTF16String& inText);

inline
void Convert(const UTF8String& inText, UTF16String& outText)
{
	outText = Convert(inText);
}

inline
void Convert(const UTF16String& inText, UTF8String& outText)
{
	outText = Convert(inText);
}

#endif // HENCODING_H
