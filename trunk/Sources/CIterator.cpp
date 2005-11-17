/*	$Id: CIterator.cpp,v 1.23 2005/09/15 10:12:08 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Sunday March 14 2004 13:57:17
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

#include <iostream>
#include <typeinfo>

#include "HStlLimits.h"

#include "CIterator.h"
#include "CBitStream.h"
#include "CDatabank.h"
#include "CTokenizer.h"

using namespace std;

// string iterators

CStrUnionIterator::CStrUnionIterator(vector<CIteratorBase*>& inIters)
//	: fCount(0)
//	, fRead(0)
{
	for (vector<CIteratorBase*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		if (*i != nil)
		{
//			fCount += (*i)->Count();
			
			CSubIter v;
			v.fIter = *i;
			if ((*i)->Next(v.fKey, v.fValue))
				fIterators.push_back(v);
		}
	}
	
	make_heap(fIterators.begin(), fIterators.end());
}

bool CStrUnionIterator::Next(std::string& outString, uint32& outValue)
{
	bool result = false;

	while (fIterators.size() > 0 and not result)
	{
//		++fRead;
		
		if (fIterators.size() > 1)
			pop_heap(fIterators.begin(), fIterators.end());
		
		string next = fIterators.back().fKey;
		if (next > outString)
		{
			outString = fIterators.back().fKey;
			outValue = fIterators.back().fValue;
			result = true;
		}

		string k = next;
		uint32 v = outValue;
		
		if (fIterators.back().fIter->Next(k, v))
		{
			fIterators.back().fKey = k;
			fIterators.back().fValue = v;
			push_heap(fIterators.begin(), fIterators.end());
		}
		else
		{
			delete fIterators.back().fIter;
			fIterators.erase(fIterators.begin() + fIterators.size() - 1);
		}
		
		while (fIterators.size() > 0 and fIterators.front().fKey <= next)
		{
			pop_heap(fIterators.begin(), fIterators.end());
			
			k = next;
			v = outValue;
			
			if (fIterators.back().fIter->Next(k, v))
			{
				fIterators.back().fKey = k;
				fIterators.back().fValue = v;
				push_heap(fIterators.begin(), fIterators.end());
			}
			else
			{
				delete fIterators.back().fIter;
				fIterators.erase(fIterators.begin() + fIterators.size() - 1);
			}
		}
	}
	
	return result;
}

// doc iterators

CDocIterator::CDocIterator()
{
}

CDocIterator::~CDocIterator()
{
}

uint32 CDocIterator::Complexity() const
{
	return 1;
}

void CDocIterator::PrintHierarchy(int inLevel) const
{
	for (uint32 i = 0; i < inLevel; ++i)
		cout << "  ";
	cout << typeid(*this).name() << endl;
}

CDocDeltaIterator::CDocDeltaIterator(CDocIterator* inOriginal,
		int32 inOffset)
	: fOriginal(inOriginal)
	, fOffset(inOffset)
{
}

CDocDeltaIterator::~CDocDeltaIterator()
{
	delete fOriginal;
}

bool CDocDeltaIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;
	if (fOriginal != nil)
	{
		assert(fOffset > 0);
		assert(inSkip == false or fOffset <= ioDoc);

		ioDoc -= fOffset;
		result = fOriginal->Next(ioDoc, inSkip);
		ioDoc += fOffset;
	}
	return result;
}
	
uint32 CDocDeltaIterator::Count() const
{
	uint32 result = 0;
	if (fOriginal != nil)
		result = fOriginal->Count();
	return result;
}

uint32 CDocDeltaIterator::Read() const
{
	uint32 result = 0;
	if (fOriginal != nil)
		result = fOriginal->Read();
	return result;
}

uint32 CDocDeltaIterator::Complexity() const
{
	uint32 result = 0;
	if (fOriginal != nil)
		result = fOriginal->Complexity();
	return result;
}

void CDocDeltaIterator::PrintHierarchy(int inLevel) const
{
	for (uint32 i = 0; i < inLevel; ++i)
		cout << "  ";
	cout << typeid(*this).name() << endl;
	
	if (fOriginal != nil)
		fOriginal->PrintHierarchy(inLevel + 1);
}

CDbDocIterator::CDbDocIterator(HStreamBase& inData,
		int64 inOffset, int64 inMax)
	: fBits(new ibit_stream(inData, inOffset))
	, fValue(-1)
	, fRead(0)
{
	fCount = ReadGamma(*fBits);
	b = CalculateB(inMax, fCount);
	n = 0;
	g = 1;
	
	while (g < b)
	{
		++n;
		g <<= 1;
	}
	g -= b;
}

CDbDocIterator::~CDbDocIterator()
{
	delete fBits;
}

bool CDbDocIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;

	while (fRead < fCount and not result)
	{
		int32 q = 0;
		int32 r = 0;
		
		if (fBits->next_bit())
		{
			q = 1;
			while (fBits->next_bit())
				++q;
		}
		
		if (b > 1)
		{
			for (int e = 0; e < n - 1; ++e)
				r = (r << 1) | fBits->next_bit();
			
			if (r >= g)
			{
				r = (r << 1) | fBits->next_bit();
				r -= g;
			}
		}
		
		int32 d = r + q * b + 1;
		
		fValue += d;
		++fRead;
		
		if (fValue > ioDoc or fValue == 0 or not inSkip)
		{
			ioDoc = static_cast<uint32>(fValue);
			result = true;
		}
	}

	return result;
}

uint32 CDbDocIterator::Read() const
{
	return fRead;
}

uint32 CDbDocIterator::Count() const
{
	return fCount;
}

CDocNrIterator::CDocNrIterator(uint32 inValue)
	: fValue(inValue)
{
}

bool CDocNrIterator::Next(uint32& ioDoc, bool)
{
	bool result = false;
	if (fValue != numeric_limits<uint32>::max())
	{
		ioDoc = fValue;
		fValue = numeric_limits<uint32>::max();
		result = true;
	}
	return result;
}

uint32 CDocNrIterator::Count() const
{
	return 1;
}

uint32 CDocNrIterator::Read() const
{
	if (fValue == numeric_limits<uint32>::max())
		return 1;
	else
		return 0;
}

CDocIterator* CDocUnionIterator::Create(std::vector<CDocIterator*>& inIters)
{
	if (inIters.size() == 0)
		return nil;
	else if (inIters.size() == 1)
		return inIters.front();
	else
		return new CDocUnionIterator(inIters);
}

CDocIterator* CDocUnionIterator::Create(CDocIterator* inFirst, CDocIterator* inSecond)
{
	if (inFirst and inSecond)
		return new CDocUnionIterator(inFirst, inSecond);
	else if (inFirst)
		return inFirst;
	else
		return inSecond;
}

CDocUnionIterator::CDocUnionIterator(vector<CDocIterator*>& inIters)
	: fCount(0)
	, fRead(0)
{
	for (vector<CDocIterator*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		if (*i)
		{
			fCount += (*i)->Count();
			
			CSubIter v;
			v.fIter = *i;
			if ((*i)->Next(v.fValue, false))
				fIterators.push_back(v);
		}
	}
	
	make_heap(fIterators.begin(), fIterators.end());
}

CDocUnionIterator::CDocUnionIterator(CDocIterator* inFirst, CDocIterator* inSecond)
	: fCount(0)
	, fRead(0)
{
	fCount = inFirst->Count() + inSecond->Count();
	
	CSubIter v;
	v.fIter = inFirst;
	if (inFirst->Next(v.fValue, false))
		fIterators.push_back(v);

	v.fValue = 0;
	v.fIter = inSecond;
	if (inSecond->Next(v.fValue, false))
		fIterators.push_back(v);
	
	make_heap(fIterators.begin(), fIterators.end());
}

bool CDocUnionIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;

	while (fIterators.size() > 0 and not result)
	{
		++fRead;
		
		if (fIterators.size() > 1)
			pop_heap(fIterators.begin(), fIterators.end());
		
		uint32 next = fIterators.back().fValue;
		if (next > ioDoc or inSkip == false)
		{
			ioDoc = fIterators.back().fValue;
			result = true;
		}
		else if (inSkip)
			next = ioDoc - 1;

		uint32 v = next;
		if (fIterators.back().fIter->Next(v, inSkip))
		{
			fIterators.back().fValue = v;
			push_heap(fIterators.begin(), fIterators.end());
		}
		else
		{
			delete fIterators.back().fIter;
			fIterators.erase(fIterators.begin() + fIterators.size() - 1);
		}
		
		while (fIterators.size() > 0 && fIterators.front().fValue <= next)
		{
			pop_heap(fIterators.begin(), fIterators.end());
			
			uint32 v = next;
			if (fIterators.back().fIter->Next(v, inSkip))
			{
				fIterators.back().fValue = v;
				push_heap(fIterators.begin(), fIterators.end());
			}
			else
			{
				delete fIterators.back().fIter;
				fIterators.erase(fIterators.begin() + fIterators.size() - 1);
			}
		}
	}
	
	return result;
}

uint32 CDocUnionIterator::Read() const
{
	return fRead;
}

uint32 CDocUnionIterator::Count() const
{
	return fCount;
}

void CDocUnionIterator::PrintHierarchy(int inLevel) const
{
	for (uint32 i = 0; i < inLevel; ++i)
		cout << "  ";
	cout << typeid(*this).name() << endl;
	
	std::vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		(*i).fIter->PrintHierarchy(inLevel + 1);
}

uint32 CDocUnionIterator::Complexity() const
{
	uint32 result = fIterators.size();
	std::vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		result += (*i).fIter->Complexity();
	return result;
}

CDocIntersectionIterator::CDocIntersectionIterator(vector<CDocIterator*>& inIters)
	: fCount(numeric_limits<uint32>::max())
	, fRead(0)
{
	for (vector<CDocIterator*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		if ((*i)->Count() < fCount)
			fCount = (*i)->Count();

		CSubIter v;
		v.fIter = *i;
		if ((*i)->Next(v.fValue, false))
			fIterators.push_back(v);
	}
	
	sort(fIterators.begin(), fIterators.end());
}

CDocIntersectionIterator::CDocIntersectionIterator(CDocIterator* inFirst, CDocIterator* inSecond)
	: fCount(0)
	, fRead(0)
{
	if (inFirst == nil or inSecond == nil)
	{
		delete inFirst;
		delete inSecond;
	}
	else
	{
		fCount = inFirst->Count();
		if (fCount > inSecond->Count())
			fCount = inSecond->Count();

		CSubIter v1, v2;
		v1.fValue = 0;
		v2.fValue = 0;
		v1.fIter = inFirst;
		v2.fIter = inSecond;
	
		if (inFirst->Next(v1.fValue, false) and
			inSecond->Next(v2.fValue, false))
		{
			fIterators.push_back(v1);
			fIterators.push_back(v2);
		}
		else
		{
			delete inFirst;
			delete inSecond;
		}
	}
	
	if (fIterators.size())
		sort(fIterators.begin(), fIterators.end());
}

bool CDocIntersectionIterator::Next(uint32& ioValue, bool inSkip)
{
	bool result = false;
	
	while (fIterators.size() > 0 and not result)
	{
		vector<CSubIter>::iterator i;

		result = true;
		uint32 next = fIterators.front().fValue;
		for (i = fIterators.begin() + 1; i != fIterators.end(); ++i)
		{
			result = result and (*i).fValue == next;
			if ((*i).fValue > next)
				next = (*i).fValue - 1;
		}
		
		if (result and next < ioValue)
			result = false;
		else
			ioValue = next;

		for (i = fIterators.begin(); i != fIterators.end(); ++i)
		{
			if ((*i).fValue <= next)
			{
				uint32 v = next;
				if (not (*i).fIter->Next(v, true))
				{
					fIterators.erase(fIterators.begin(), fIterators.end());
					break;
				}
				else
					(*i).fValue = v;
			}
		}
		
		if (fIterators.size())
			sort(fIterators.begin(), fIterators.end());
	}

	if (result)
		++fRead;
	
	return result;
}

uint32 CDocIntersectionIterator::Read() const
{
	return fRead;
}

uint32 CDocIntersectionIterator::Count() const
{
	return fCount;
}

uint32 CDocIntersectionIterator::Complexity() const
{
	uint32 result = fIterators.size();

	vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		result += (*i).fIter->Complexity();
	
	return result;
}

void CDocIntersectionIterator::PrintHierarchy(int inLevel) const
{
	for (uint32 i = 0; i < inLevel; ++i)
		cout << "  ";
	cout << typeid(*this).name() << endl;
	
	std::vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		(*i).fIter->PrintHierarchy(inLevel + 1);
}

CDocNotIterator::CDocNotIterator(
		CDocIterator* inIter, uint32 inMax)
	: fIter(inIter)
	, fCur(0)
	, fNext(0)
	, fMax(inMax)
	, fRead(0)
{
	if (not fIter->Next(fNext, false))
		fNext = fMax;
}

bool CDocNotIterator::Next(uint32& ioValue, bool inSkip)
{
	bool result = false;

	while (not result)
	{
		if (fCur == fNext)
		{
			if (fNext < fMax and not fIter->Next(fNext, false))
				fNext = fMax;
		}
		else if (fCur < fMax)
		{
			result = not inSkip or fCur > ioValue;
			++fRead;
			if (result)
				ioValue = fCur;
		}
		else
			break;

		++fCur;
	}
	
	return result;
}

uint32 CDocNotIterator::Count() const
{
	return fMax - fIter->Count();
}

uint32 CDocNotIterator::Read() const
{
	return fRead;
}

uint32 CDocNotIterator::Complexity() const
{
	return fIter->Complexity() + 1;
}

void CDocNotIterator::PrintHierarchy(int inLevel) const
{
	for (uint32 i = 0; i < inLevel; ++i)
		cout << "  ";
	cout << typeid(*this).name() << endl;
	
	fIter->PrintHierarchy(inLevel + 1);
}

CDbAllDocIterator::CDbAllDocIterator(uint32 inMaxDocNr)
	: fDocNr(0)
	, fMaxDocNr(inMaxDocNr)
{
}

CDbAllDocIterator::~CDbAllDocIterator()
{
}

bool CDbAllDocIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;
	if (fDocNr < fMaxDocNr)
	{
		ioDoc = fDocNr;
		++fDocNr;
		result = true;
	}
	return result;
}

uint32 CDbAllDocIterator::Count() const
{
	return fMaxDocNr;
}

uint32 CDbAllDocIterator::Read() const
{
	return fDocNr;
}

// ---------------------------------------------------------------------------

CDbStringMatchIterator::CDbStringMatchIterator(CDatabankBase& inDb, const std::vector<std::string>& inStringWords,
		CDocIterator* inBaseIterator)
	: fDb(inDb)
	, fStringWords(inStringWords)
	, fIter(inBaseIterator)
{
}

CDbStringMatchIterator::~CDbStringMatchIterator()
{
	delete fIter;
}

bool CDbStringMatchIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool found = false;
	
	while (not found and fIter != nil and fIter->Next(ioDoc, inSkip))
	{
		string doc = fDb.GetDocument(ioDoc);
		
		int64 backTrace = 0;
		uint32 ix = 0;
		deque<string> buffer;
		
		CTokenizer<255> tok(doc.c_str(), doc.length());
		bool isWord, isNumber;
		CTokenizer<255>::EntryText s;
		vector<string> stringWords;
		
		// KLUDGE ALERT!!!
		// since we're indexing these weird, low tech flat file
		// databanks we might have a field id in between our string
		// words in the original data... This really sucks...
		// 
		// The real solution is to change the way data is stored. :-(
		
		while (tok.GetToken(s, isWord, isNumber))
		{
			if (not (isWord or isNumber) or s[0] == 0)
				continue;

			char* c;
			for (c = s; *c; ++c)
				*c = static_cast<char>(tolower(*c));
			
			if (fStringWords[ix] == s)
			{
				++ix;
				
				if (ix == fStringWords.size())
				{
					found = true;
					break;
				}
				else if (ix == 1)
					backTrace = tok.GetOffset();
			}
			else if (ix > 0)
			{
				// ok, so we've found a word that doesn't fit in our string
				// see if the character before this word is a newline.
				// If so, we simply skip this word assuming it is a leading
				// field identifier.
				
				int64 offset = tok.GetOffset() - (c - s) - 1;
				if (offset < 0 or doc[offset] != '\n')
				{
					ix = 0;
					tok.SetOffset(backTrace);
				}
			}
			else
				backTrace = tok.GetOffset();
		}
	}
	
	return found;
}

uint32 CDbStringMatchIterator::Count() const
{
	uint result = 0;
	if (fIter != 0)
		result = fIter->Count();
	return result;
}

uint32 CDbStringMatchIterator::Read() const
{
	uint result = 0;
	if (fIter != 0)
		result = fIter->Read();
	return result;
}



