/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Friday May 27 2005 15:47:12
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
 
#include "MRS.h"

#include "HStlLimits.h"
#include "HStdCCtype.h"
#include "HStlIOStream.h"
#include "HStlIOManip.h"

#include "HError.h"

#include "CSequence.h"
#include "CMatrix.h"

using namespace std;

class AATable
{
  public:
	static AATable&		Instance();
	
	uint8				operator[](char inAA) const;
	char				operator[](uint8 inCode) const;

  private:
						AATable();

	uint8				mMap[256];
};

AATable::AATable()
{
	for (uint32 c = numeric_limits<uint8>::min(); c <= numeric_limits<uint8>::max(); ++c)
	{
		mMap[c] = kUnknownCode;
		
		for (int code = 0; code < kAACodeCount; ++code)
		{
			if (toupper(c) == kAACodes[code])
				mMap[c] = code;
		}
	}
}

AATable& AATable::Instance()
{
	static AATable sInstance;
	return sInstance;
}

inline uint8 AATable::operator[](char inAA) const
{
	return mMap[static_cast<uint8>(inAA)];
}

inline char AATable::operator[](uint8 inCode) const
{
	char result = '_';
	if (inCode < kAACodeCount)
		result = kAACodes[inCode];
	return result;
}

uint8 Encode(char inAA)
{
	AATable& aaTable = AATable::Instance();
	return aaTable[inAA];
}

CSequence Encode(const string& inAASequence)
{
	CMutableSequence result;
	
	AATable& aaTable = AATable::Instance();
	
	for (string::const_iterator aai = inAASequence.begin(); aai != inAASequence.end(); ++aai)
		result += aaTable[*aai];
	
	return CSequence(result.begin(), result.end());
}

string Decode(CSequence inSequence)
{
	string result;
	for (CSequence::iterator i = inSequence.begin(); i != inSequence.end(); ++i)
		result.push_back(Decode(*i));
	return result;
}

void PrintAlignment(
	const CSequence& inQuery, uint32 inQueryStart,
	const CSequence& inTarget, uint32 inTargetStart)
{
	assert(inQuery.length() == inTarget.length());
	
	CMutableSequence q(inQuery.begin(), inQuery.end());
	CMutableSequence s(inTarget.begin(), inTarget.end());
	
	while (q.length())
	{
		uint32 n = q.length();
		if (n > 60)
			n = 60;
		
		cout << setw(6) << inQueryStart << " ";
		for (uint32 i = 0; i < n; ++i)
		{
			if (q[i] == '-')
				cout << '-';
			else
			{
				cout << Decode(q[i]);
				++inQueryStart;
			}
		}
		cout << endl;

		cout << "       ";
		for (uint32 i = 0; i < n; ++i)
		{
			if (s[i] == q[i])
				cout << '|';
			else
				cout << " ";
		}
		cout << endl;

		cout << setw(6) << inTargetStart << " ";
		for (uint32 i = 0; i < n; ++i)
		{
			if (s[i] == '-')
				cout << '-';
			else
			{
				cout << Decode(s[i]);
				++inTargetStart;
			}
		}
		cout << endl << endl;
		
		q.erase(q.begin(), q.begin() + n);
		s.erase(s.begin(), s.begin() + n);
	}
	cout << endl;
}

void MidLine(const CSequence& inQuery, const CSequence& inTarget,
	const CMatrix& inMatrix, uint32& outIdentity, uint32& outPositives,
	uint32& outGaps, string& outMidline)
{
	outMidline.clear();
	outIdentity = 0;
	outPositives = 0;
	outGaps = 0;
	
	assert(inQuery.length() == inTarget.length());
	if (inQuery.length() != inTarget.length())
		THROW(("To calculate a midline you need a query and a target of the same length"));

	for (CSequence::const_iterator a = inQuery.begin(), b = inTarget.begin();
		a != inQuery.end(); ++a, ++b)
	{
		if (*a == *b)
		{
			++outIdentity;
			++outPositives;
			outMidline += Decode(*a);
		}
		else if (inMatrix(*a, *b) > 0)
		{
			++outPositives;
			outMidline += '+';
		}
		else
		{
			if (*a == kSignalGapCode or *b == kSignalGapCode)
				++outGaps;
				
			outMidline += ' ';
		}
	}	
}
