/*	$Id: HFile.cpp,v 1.18 2005/08/22 12:38:04 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday September 12 2000 09:03:00
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
#include "HStdCString.h"

#include "HFile.h"
#include "HDefines.h"
#include "HUtils.h"
//#include "HStrings.h"
#include "HError.h"

using namespace std;

HFileExt::HFileExt()
{
}

HFileExt::HFileExt(const HFileExt& inOther)
	: fDesc(inOther.fDesc)
	, fExt(inOther.fExt)
	, fDefaultExtension(inOther.fDefaultExtension)
{
}

HFileExt::HFileExt(string inDesc, string inExt,
					string inDefaultExtension)
	: fDesc(inDesc)
	, fExt(inExt)
	, fDefaultExtension(inDefaultExtension)
{
}

void HFileExt::SetDescription(string inDesc)
{
	fDesc = inDesc;
}

string	HFileExt::GetDescription() const
{
	return fDesc;
}

void HFileExt::SetExtensions(string inExt)
{
	fExt = inExt;
}

string HFileExt::GetExtensions() const
{
	return fExt;
}

string HFileExt::GetDefaultExtension() const
{
	return fDefaultExtension;
}

static bool Match(const char* inPattern, const char* inName)
{
	for (;;)
	{
		char op = *inPattern;

		switch (op)
		{
			case 0:
				return *inName == 0;
			case '*':
			{
				if (inPattern[1] == 0)	// last '*' matches all 
					return true;

				const char* n = inName;
				while (*n)
				{
					if (Match(inPattern + 1, n))
						return true;
					++n;
				}
				return false;
			}
			case '?':
				if (*inName)
					return Match(inPattern + 1, inName + 1);
				else
					return false;
			default:
				if (tolower(*inName) == tolower(op))
				{
					++inName;
					++inPattern;
				}
				else
					return false;
				break;
		}
	}
}

bool HFileExt::Matches(string inName) const
{
	bool match = false;

	if (inName.length() > 0)
	{
		HAutoBuf<char> b(new char[fExt.length() + 1]);
		strcpy(b.get(), fExt.c_str());
	
		char* pat = strtok(b.get(), ";");
		while (not match && pat != nil)
		{
			if (strlen(pat) > 0)
				match = Match(pat, inName.c_str());
		
			pat = strtok(nil, ";");
		}
	}
	
	return match;
}

bool HFileExt::Matches(const HUrl& inUrl) const
{
	return Matches(inUrl.GetFileName());
}

bool HFileExt::operator==(const HFileExt& inOther) const
{
	return fDesc == inOther.fDesc;
}

#pragma mark -

namespace HFile
{

int HErrno;

HExtensionSet&
HExtensionSet::Instance()
{
	static HExtensionSet sInstance;
	
	if (sInstance.size() == 0)
		sInstance.Init();
	
	return sInstance;
}

HExtensionSet::HExtensionSet()
{
}

void
HExtensionSet::Init()
{
		// clear the contents
	HExtensionSet e;
	swap(e);

#if MINI_H_LIB
		// and add the default pattern
	push_back(HFileExt("Text files", "*.txt;*.text"));
	push_back(HFileExt("All files", "*"));
#else
		// and add the default pattern
	push_back(HFileExt(HStrings::GetIndString(7000, 0), "*.txt;*.text"));
	push_back(HFileExt(HStrings::GetIndString(7000, 1), "*"));
#endif
}

void
HExtensionSet::GetFilter(string& outFilter)
{
	for (iterator i = begin(); i != end(); ++i)
	{
		outFilter += (*i).GetDescription();
		outFilter += static_cast<char>('\0');
		outFilter += "*";
		outFilter += (*i).GetExtensions();
		outFilter += static_cast<char>('\0');
	}

	outFilter += static_cast<char>('\0');
	outFilter += static_cast<char>('\0');
}

int HExtensionSet::GetIndex(string inName) const
{
	int result = 1;
	
	for (const_iterator i = begin(); i != end(); ++i)
	{
		if ((*i).Matches(inName))
		{
			result = i - begin();
			break;
		}
		
		if (i == begin())	// skip over second
			++i;
	}
	return result;
}

int HExtensionSet::Count() const
{
	return static_cast<int>(size()) + 1;
}

HFileExt HExtensionSet::operator[] (int inIndex) const
{
	const_iterator i = begin();
	while (i != end() && inIndex-- > 0)
		++i;
	
	ThrowIf(i == end());
	return *i;
}

void AddTextFileNameExtension(const HFileExt& inExt)
{
	HExtensionSet& es = HExtensionSet::Instance();
#if P_DEBUG
	for (HExtensionSet::iterator i = es.begin(); i != es.end(); ++i)
		assert(not (*i == inExt));
#endif

	es.push_back(inExt);
}

void ClearTextFileNameExtensionMapping()
{
	HExtensionSet::Instance().Init();
}

struct GlobImp
{
							GlobImp(FileIterator* inIter, string inPattern)
								: fIter(inIter)
								, fPattern(inPattern)
							{
							}
	
	HAutoPtr<FileIterator>	fIter;
	string					fPattern;
};

GlobIterator::GlobIterator(string inGlobPattern)
	: fImpl(nil)
{
	HUrl url(inGlobPattern);
	
	vector<string> pat;
	
	string nPat = url.GetFileName();
	url = url.GetParent();
	
	while (url.GetURL() != "file:///")
	{
		string name = url.GetFileName();
		
		if (name.find('*') != string::npos or
			name.find('?') != string::npos)
		{
			pat.push_back(name);
			url = url.GetParent();
		}
		else
			break;
	}

	url.NormalizePath();
	
	fImpl = new GlobImp(new FileIterator(url, pat.size() > 0), inGlobPattern);
}

GlobIterator::~GlobIterator()
{
	delete fImpl;
}

bool GlobIterator::Next(HUrl& outUrl)
{
	bool result = false;
	
	while (not result and fImpl->fIter->Next(outUrl))
	{
		string url = outUrl.GetURL();
		HUrl::DecodeReservedChars(url);
		
		result = Match(fImpl->fPattern.c_str(), url.c_str());
	}
	
	return result;
}

}
