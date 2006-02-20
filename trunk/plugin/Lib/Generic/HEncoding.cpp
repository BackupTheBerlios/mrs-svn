/*	$Id$
	Copyright hekkelman
	Created Wednesday January 18 2006 13:44:37
*/

#include "HLib.h"

#include "HEncoding.h"

UTF16String Convert(const UTF8String& inText)
{
	UTF16String result;

	UTF8String::const_iterator c = inText.begin();

	while (c != inText.end())
	{
		uint32 uc = 0x0FFFD;
		uint32 length = 1;
		
		if ((*c & 0x080) == 0)
		{
			uc = static_cast<uint32>(*c);
		}
		else if ((*c & 0x0E0) == 0x0C0)
		{
			if ((*(c + 1) & 0x00C0) == 0x0080)
			{
				uc = static_cast<uint32>(((*c & 0x01F) << 6) | (*(c + 1) & 0x03F));
				length = 2;
			}
		}
		else if ((*c & 0x0F0) == 0x0E0)
		{
			if ((*(c + 1) & 0x000c0) == 0x00080 and (*(c + 2) & 0x000c0) == 0x00080)
			{
				uc = static_cast<uint32>(((*c & 0x00F) << 12) | ((*(c + 1) & 0x03F) << 6) | (*(c + 2) & 0x03F));
				length = 3;
			}
		}
		else if ((*c & 0x0F8) == 0x0F0)
		{
			if ((*(c + 1) & 0x000c0) != 0x00080 or (*(c + 2) & 0x000c0) != 0x00080 or
				(*(c + 3) & 0x000c0) != 0x00080)
			{
				uc = static_cast<uint32>(((*c & 0x007) << 18) | ((*(c + 1) & 0x03F) << 12) | ((*(c + 2) & 0x03F) << 6) | (*(c + 3) & 0x03F));
				length = 4;
			}
		}
		
		while (c != inText.end() and length-- > 0)
			++c;
		
		if (uc <= 0xFFFF)
		{
			// assume native byte ordering
			result += static_cast<UTF16String::value_type>(uc);
		}
		else
		{
			UTF16String::value_type h = (uc - 0x10000) / 0x400 + 0xD800;
			UTF16String::value_type l = (uc - 0x10000) % 0x400 + 0xDC00;
			
			result += h;
			result += l;
		}
		
	}
	
	return result;
}

UTF8String Convert(const UTF16String& inText)
{
	UTF8String result;
	
	UTF16String::const_iterator c = inText.begin();
	
	while (c != inText.end())
	{
		uint32 uc = *c++;
		if (uc >= 0xD800 and uc <= 0xDBFF and c + 1 != inText.end() and *(c + 1) >= 0xDC00 and *(c + 1) <= 0xDFFF)
		{
			++c;
			uc = (uc - 0xD800) * 0x400 + (*c - 0xDC00) + 0x10000;
		}
		
		if (uc < 0x080)
		{
			result += static_cast<char> (uc);
		}
		else if (uc < 0x0800)
		{
			result += static_cast<char> (0x0c0 | (uc >> 6));
			result += static_cast<char> (0x080 | (uc & 0x3f));
		}
		else if (uc < 0x00010000)
		{
			result += static_cast<char> (0x0e0 | (uc >> 12));
			result += static_cast<char> (0x080 | ((uc >> 6) & 0x3f));
			result += static_cast<char> (0x080 | (uc & 0x3f));
		}
		else
		{
			result += static_cast<char> (0x0f0 | (uc >> 18));
			result += static_cast<char> (0x080 | ((uc >> 12) & 0x3f));
			result += static_cast<char> (0x080 | ((uc >> 6) & 0x3f));
			result += static_cast<char> (0x080 | (uc & 0x3f));
		}
	}
	
	return result;
}

