/*	$Id: HStrings.cpp,v 1.21 2005/08/22 12:38:04 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created Wednesday October 10 2001 08:31:47
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

#include "HStdCString.h"
#include "HStlSet.h"

#include "HStrings.h"
#include "HError.h"
#include "HResources.h"
#include "HStream.h"
#include "HUtils.h"
#include "HFile.h"

#include "expat.h"

#if P_DEBUG
#include <iostream>
#endif

namespace HStrings {

using namespace std;

struct HLocalisedString
{
	uint32	res_type;
	int16	res_id;
//	uint16	crc;
	string	orig;
	string	loc;
	
	bool operator<(const HLocalisedString& inOther) const
	{
		int32 d = res_type - inOther.res_type;
		if (d == 0)
			d = res_id - inOther.res_id;
		if (d == 0)
//			d = crc - inOther.crc;
			d = orig.compare(inOther.orig);
		return d < 0;
	}
};

typedef set<HLocalisedString> HLocalSet;

typedef void (*CharHandler)(string inData);

static CharHandler	gCharHandler;

static uint32		gRsrcType;
static int32		gRsrcID;
static string		gOriginal, gLocalised;
static HLocalSet	gStrings;

static void PushOriginal(string inOriginal)
{
	gOriginal += inOriginal;
}

static void PushLocalised(string inLocalised)
{
	gLocalised += inLocalised;
}

static void startElement(void* /*userData*/, const char* name, const char** atts)
{
	ThrowIfNil(name);
	
	string n = name;
	
	if (n == "res")
	{
		for (int i = 0; atts[i]; i += 2)
		{
			if (strcmp(atts[i], "type") == 0)
			{
				gRsrcType = 0;
				
				const char* t = atts[i + 1];
				for (int i = 0; i < 4; ++i)
					gRsrcType = (gRsrcType << 8) | *t++;
			}
			else if (strcmp(atts[i], "id") == 0)
			{
				gRsrcID = strtol(atts[i + 1], nil, 10);
			}
		}
	}
	else if (n == "orig")
	{
		gOriginal = kEmptyString;
		gCharHandler = PushOriginal;
	}
	else if (n == "tran")
	{
		gLocalised = kEmptyString;
		gCharHandler = PushLocalised;
	}
}

static void endElement(void */*userData*/, const char *name)
{
	if (gCharHandler == PushLocalised && gLocalised != gOriginal)
	{
		HLocalisedString s;
		s.res_type = gRsrcType;
		s.res_id = gRsrcID;
//		s.crc = CalculateCRC(gOriginal.c_str(), gOriginal.length(), 0);
		s.orig = gOriginal;
		s.loc = gLocalised;
		
		if (not gStrings.insert(s).second)
			PRINT(("Overwriting localised string %s in %4.4s,%d\n", gLocalised.c_str(), &gRsrcType, gRsrcID));
	}
	
	gCharHandler = nil;
}

static void charData(void */*userData*/, const XML_Char *s, int len)
{
	if (s != nil && gCharHandler != nil)
	{
		string str(s, len);
		(*gCharHandler)(str);
	}
}

void LoadLocalisedStringTable()
{
	HUrl url = gUserURL.GetChild("localised.xml");
	if (HFile::Exists(url))
	{
		try
		{
			HBufferedFileStream file(url, O_RDONLY);

			XML_Parser p = XML_ParserCreate(nil);
			ThrowIfNil(p);

			gStrings.clear();

			XML_SetElementHandler(p, startElement, endElement);
			XML_SetCharacterDataHandler(p, charData);
			
			int32 size = static_cast<int32>(file.Size());
			const int32 kBufferSize = 8192;
			
			HAutoBuf<char> txt(new char[kBufferSize]);
			while (size > 0)
			{
				uint32 k = size;
				if (k > kBufferSize)
					k = kBufferSize;
				
				file.Read(txt.get(), k);
				size -= k;
				
				if (XML_Parse(p, txt.get(), k, size == 0) == 0 /*XML_STATUS_ERROR*/)
					THROW(("Localisation Error, XML file is probably corrupt"));
			}
		}
		catch (std::exception& e)
		{
			DisplayError(e);
			
			gStrings.clear();
		}
	}
}

struct HStringResource {
	short	string_cnt;
	char	data[1];
};

string GetIndString(int inResID, int inIndex)
{
	const HStringResource* res =
		reinterpret_cast<const HStringResource*>(
			HResources::GetResource('str#', inResID));

	const char* r = nil;
	
	if (res != nil && inIndex < res->string_cnt && inIndex >= 0)
	{
		r = res->data;
		
		while (inIndex-- > 0)
			r += strlen(r) + 1;
	}
	
	string result;
	if (r != nil)
		result = r;
	else
		PRINT(("Missing string: %d:%d", inResID, inIndex));
	
	if (gStrings.size())
		TranslateString('str#', inResID, result);
	
	return result;
}

string GetErrString(int inResID)
{
	string result;
	
	const char* s = reinterpret_cast<const char*>(
		HResources::GetResource('PErr', inResID));

	if (s != nil)
		result = s;
	else
	{
		result = "Error (unknown error string)";
#if P_DEBUG
		std::cerr << "Error retrieving error string resource: " << inResID << std::endl;
#endif
	}

	if (gStrings.size())
		TranslateString('PErr', inResID, result);
	
	return result;
}

string GetFormattedIndString(int inResID, int inIndex,
	string inParam1, string inParam2,
	string inParam3, string inParam4)
{
	string result = GetIndString(inResID, inIndex);

	string::size_type p;
	if ((p = result.find("^0")) != string::npos)
		result.replace(p, 2, inParam1);
	if ((p = result.find("^1")) != string::npos)
		result.replace(p, 2, inParam2);
	if ((p = result.find("^2")) != string::npos)
		result.replace(p, 2, inParam3);
	if ((p = result.find("^3")) != string::npos)
		result.replace(p, 2, inParam4);
	
	return result;
}

string GetFormattedErrString(int inResID,
	string inParam)
{
	string result = GetErrString(inResID);

	string::size_type p;
	if ((p = result.find("^0")) != string::npos)
		result.replace(p, 2, inParam);
	
	return result;
}

void TranslateString(uint32 inResType, int inResID, std::string& ioString)
{
	if (gStrings.size())
	{
		HLocalisedString s;
		s.res_type = inResType;
		s.res_id = inResID;
//		s.crc = CalculateCRC(ioString.c_str(), ioString.length(), 0);
		s.orig = ioString;
		
		HLocalSet::iterator i = gStrings.find(s);
		if (i != gStrings.end())
			ioString = (*i).loc;
	}
}

}
