/*	$Id$
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

#include <limits>
#include "HUtils.h"

#include "CDocIterator.h"
#include "CBitStream.h"
#include "CDatabank.h"
#include "CTokenizer.h"

using namespace std;

CDocIterator::CDocIterator()
{
}

CDocIterator::~CDocIterator()
{
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

void CDocDeltaIterator::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	if (fOriginal != nil)
		fOriginal->PrintHierarchy(inLevel + 1);
}

// ----------------------------------------------------------------------------
//
//	CDocNotIterator
//

CDocNrIterator::CDocNrIterator(uint32 inValue)
	: fValue(inValue)
{
}

bool CDocNrIterator::Next(uint32& ioDoc, bool)
{
	bool result = false;
	if (fValue != kInvalidDocID)
	{
		ioDoc = fValue;
		fValue = kInvalidDocID;
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
	if (fValue == kInvalidDocID)
		return 1;
	else
		return 0;
}

// ----------------------------------------------------------------------------
//
//	CDocUnionIterator
//

CDocIterator* CDocUnionIterator::Create(vector<CDocIterator*>& inIters)
{
	CDocIterator* result = nil;
	
	inIters.erase(
		remove(inIters.begin(), inIters.end(), static_cast<CDocIterator*>(nil)),
		inIters.end());
	
	if (inIters.size() == 1)
		result = inIters.front();
	else if (inIters.size() != 0)
		result = new CDocUnionIterator(inIters);

	return result;
}

CDocIterator* CDocUnionIterator::Create(CDocIterator* inFirst, CDocIterator* inSecond)
{
	CDocIterator* result = nil;
	
	if (inFirst and inSecond)
	{
		vector<CDocIterator*> v;
		v.push_back(inFirst);
		v.push_back(inSecond);
		result = new CDocUnionIterator(v);
	}
	else if (inFirst)
		result = inFirst;
	else
		result = inSecond;
	
	return result;
}

CDocUnionIterator::CDocUnionIterator(vector<CDocIterator*>& inIters)
	: fCount(0)
	, fRead(0)
{
	for (vector<CDocIterator*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		fCount += (*i)->Count();

		if (dynamic_cast<CDocUnionIterator*>(*i) != nil)
		{
			CDocUnionIterator* u = static_cast<CDocUnionIterator*>(*i);
			fIterators.insert(fIterators.end(), u->fIterators.begin(), u->fIterators.end());
			u->fIterators.erase(u->fIterators.begin(), u->fIterators.end());
			delete u;
		}
		else
		{
			CSubIter v;
			v.fIter = *i;
			if ((*i)->Next(v.fValue, false))
				fIterators.push_back(v);
			else
				delete *i;
		}
	}
	
	make_heap(fIterators.begin(), fIterators.end());
}

CDocUnionIterator::~CDocUnionIterator()
{
	for (vector<CSubIter>::iterator i = fIterators.begin(); i != fIterators.end(); ++i)
		delete i->fIter;
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
		
		while (fIterators.size() > 0 and fIterators.front().fValue <= next)
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
	CDocIterator::PrintHierarchy(inLevel);
	
	vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		(*i).fIter->PrintHierarchy(inLevel + 1);
}

struct CleanUpIter { void operator()(CDocIterator*& iter) const { delete iter; } };

CDocIterator* CDocIntersectionIterator::Create(std::vector<CDocIterator*>& inIters)
{
	CDocIterator* result = nil;
	if (find(inIters.begin(), inIters.end(), static_cast<CDocIterator*>(nil)) == inIters.end())
		result = new CDocIntersectionIterator(inIters);
	else
		for_each(inIters.begin(), inIters.end(), CleanUpIter());
	
	return result;
}

CDocIterator* CDocIntersectionIterator::Create(CDocIterator* inFirst, CDocIterator* inSecond)
{
	CDocIterator* result = nil;

	if (inFirst and inSecond)
	{
		vector<CDocIterator*> v;
		v.push_back(inFirst);
		v.push_back(inSecond);
		result = new CDocIntersectionIterator(v);
	}

	return result;
}

CDocIntersectionIterator::CDocIntersectionIterator(vector<CDocIterator*>& inIters)
	: fCount(kInvalidDocID)
	, fRead(0)
{
	for (vector<CDocIterator*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		if ((*i)->Count() < fCount)
			fCount = (*i)->Count();

		if (dynamic_cast<CDocIntersectionIterator*>(*i) != nil)
		{
			CDocIntersectionIterator* o = static_cast<CDocIntersectionIterator*>(*i);
			fIterators.insert(fIterators.end(), o->fIterators.begin(), o->fIterators.end());
			o->fIterators.erase(o->fIterators.begin(), o->fIterators.end());
			delete o;
		}
		else
		{
			CSubIter v;
			v.fIter = *i;
			if (not (*i)->Next(v.fValue, false))
				v.fValue = kInvalidDocID;
			
			fIterators.push_back(v);
		}
	}
	
	sort(fIterators.begin(), fIterators.end());
}

CDocIntersectionIterator::~CDocIntersectionIterator()
{
	for (vector<CSubIter>::iterator i = fIterators.begin(); i != fIterators.end(); ++i)
		delete i->fIter;
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
		
		if (result /*and inSkip*/ and next < ioValue)
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
					for (vector<CSubIter>::iterator i = fIterators.begin(); i != fIterators.end(); ++i)
						delete i->fIter;
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

void CDocIntersectionIterator::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		(*i).fIter->PrintHierarchy(inLevel + 1);
}

CDocNotIterator::CDocNotIterator(CDocIterator* inIter, uint32 inMax)
	: fIter(inIter)
	, fCur(0)
	, fNext(0)
	, fMax(inMax)
	, fRead(0)
{
	if (fIter == nil or not fIter->Next(fNext, false))
		fNext = fMax;
}

CDocNotIterator::~CDocNotIterator()
{
	delete fIter;
}

bool CDocNotIterator::Next(uint32& ioValue, bool inSkip)
{
	bool result = false;

	while (not result)
	{
		if (fCur == fNext)
		{
			if (fNext < fMax and (fIter == nil or not fIter->Next(fNext, false)))
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
	uint32 result = fMax;
	if (fIter != nil)
		result -= fIter->Count();
	return result;
}

uint32 CDocNotIterator::Read() const
{
	return fRead;
}

void CDocNotIterator::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	if (fIter != nil)
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

CDocVectorIterator::CDocVectorIterator(const DocFreqVector& inDocs)
	: fDocs(new DocFreqVector(inDocs))
	, fCur(0)
	, fRead(0)
{
}

bool CDocVectorIterator::Next(uint32& ioValue, bool inSkip)
{
	bool result = false;
	while (not result and fCur < fDocs->size())
	{
		if (fDocs->at(fCur).first <= ioValue and inSkip)
		{
			++fCur;
			continue;
		}
		
		result = true;
		ioValue = fDocs->at(fCur).first;
		++fCur;
	}
	return result;
}

bool CDocVectorIterator::Next(uint32& ioDoc, float& outFreq, bool inSkip)
{
	bool result = false;
	while (not result and fCur < fDocs->size())
	{
		if (fDocs->at(fCur).first <= ioDoc and inSkip)
		{
			++fCur;
			continue;
		}
		
		result = true;
		ioDoc = fDocs->at(fCur).first;
		outFreq = fDocs->at(fCur).second;
		++fCur;
	}
	return result;
}

// ---------------------------------------------------------------------------

CSortDocIterator::CSortDocIterator(CDocIterator* inDocIterator, uint32 inSizeHint)
	: fCur(0)
	, fRead(0)
{
	auto_ptr<CDocIterator> iter(inDocIterator);
	
	if (inDocIterator != nil)
	{
		if (inSizeHint)
			fDocs.reserve(inSizeHint);
		else
			fDocs.reserve(iter->Count());
		
		uint32 d;
		while (iter->Next(d, false))
			fDocs.push_back(d);
		
		sort(fDocs.begin(), fDocs.end());
	}
}
	
bool CSortDocIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;
	while (not result and fCur < fDocs.size())
	{
		if (fDocs[fCur] <= ioDoc and inSkip)
		{
			++fCur;
			continue;
		}
		
		result = true;
		ioDoc = fDocs[fCur];
		++fCur;
	}
	return result;
}

// ---------------------------------------------------------------------------

CDbStringMatchIterator::CDbStringMatchIterator(CDatabankBase& inDb, const vector<string>& inStringWords,
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
	uint32 result = 0;
	if (fIter != 0)
		result = fIter->Count();
	return result;
}

uint32 CDbStringMatchIterator::Read() const
{
	uint32 result = 0;
	if (fIter != 0)
		result = fIter->Read();
	return result;
}

CDocCachedIterator::CDocCachedIterator(CDocIterator* inIterator, uint32 inCache)
	: fIter(inIterator)
	, fPointer(0)
	, fCacheContainsAll(false)
{
	double start = system_time();
	double kLimitTime = 0.1;		// limit to a tenth of a second

	uint32 v = 0, n = 0;
	
	while (n++ < 1000 or start + kLimitTime > system_time())
	{
		if (fIter->Next(v, false))
			fCache.push_back(v);
		else
		{
			fCacheContainsAll = true;
			break;
		}
	}
}

bool CDocCachedIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;
	
	if (fPointer >= fCache.size() and fIter.get())
	{
		uint32 v = ioDoc;
		if (fCache.size() > 0 and inSkip and fCache.back() > v)
			v = fCache.back();
		if (fIter->Next(v, inSkip))
			fCache.push_back(v);
	}

	if (fPointer < fCache.size())
	{
		ioDoc = fCache[fPointer];
		++fPointer;
		result = true;
	}

	return result;
}

uint32 CDocCachedIterator::Count() const
{
	return fIter->Count();
}

uint32 CDocCachedIterator::Read() const
{
	return fIter->Read();
}

void CDocCachedIterator::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	if (fIter.get() != nil)
		fIter->PrintHierarchy(inLevel + 1);
}

// the merged db doc iterator

CMergedDbDocIterator::CMergedDbDocIterator(
		CDbDocIteratorBase* inIterA, uint32 inDeltaA, uint32 inDbCountA,
		CDbDocIteratorBase* inIterB, uint32 inDeltaB, uint32 inDbCountB)
	: fCount(0)
	, fRead(0)
{
	CSubIter v;
	
	if (inIterA != nil and inIterA->Next(v.fDocNr, v.fRank, false))
	{
		v.fDelta = inDeltaA;
		v.fIter = inIterA;
		fCount += inIterA->Count();
		fIterators.push_back(v);
	}
	else
		delete inIterA;
	
	if (inIterB != nil and inIterB->Next(v.fDocNr, v.fRank, false))
	{
		v.fDelta = inDeltaB;
		v.fIter = inIterB;
		fCount += inIterB->Count();
		fIterators.push_back(v);
	}
	else
		delete inIterB;
	
	make_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());

	fIDFCorrectionFactor = log(1.0 + static_cast<double>(inDbCountA + inDbCountB) / fCount);
}

bool CMergedDbDocIterator::Next(uint32& ioDoc, bool inSkip)
{
	uint8 r;
	return Next(ioDoc, r, inSkip);
}

bool CMergedDbDocIterator::Next(uint32& ioDoc, uint8& ioRank, bool inSkip)
{
	bool result = false;

	while (fIterators.size() > 0)
	{
		pop_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());
		
		CSubIter& si = fIterators.back();
		
		uint32 d = si.fDocNr + si.fDelta;
		uint8 r = si.fRank;
		
		if (si.fIter->Next(si.fDocNr, si.fRank, inSkip))
			push_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());
		else
		{
			delete si.fIter;
			fIterators.erase(fIterators.end() - 1);
		}
		
		if (inSkip and d <= ioDoc)
			continue;
		
		ioDoc = d;
		ioRank = r;
		
		result = true;
		++fRead;
		break;
	}
	
	return result;
}

// ----------------------------------------------------------------------------
//
//	Joined db support, simply merge the passed in iterators adding an offset if needed
//

CDbJoinedIterator::CDbJoinedIterator()
	: fDocCount(0)
	, fCount(0)
	, fRead(0)
{
}

CDbJoinedIterator::~CDbJoinedIterator()
{
}

void CDbJoinedIterator::AddIterator(CDbDocIteratorBase* inIter, uint32 inCount)
{
	fCount += inIter->Count();

	CSubIter si;
	si.fOffset = fDocCount;
	si.fIter = inIter;

	if (inIter->Next(si.fDocNr, si.fRank, false))
	{
		si.fDocNr += si.fOffset;
		fIterators.push_back(si);
		
		make_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());
	}
	else
		delete inIter;

	fDocCount += inCount;
	
	fIDFCorrectionFactor = log(1.0 + static_cast<double>(fDocCount) / fCount);
}

bool CDbJoinedIterator::Next(uint32& ioDoc, bool inSkip)
{
	THROW(("Should not be called"));
	return false;
}

bool CDbJoinedIterator::Next(uint32& ioDoc, uint8& outRank, bool inSkip)
{
	bool result = false;

	while (fIterators.size() > 0 and not result)
	{
		++fRead;
		
		if (fIterators.size() > 1)
		{
			pop_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());
			
			result = true;
			ioDoc = fIterators.back().fDocNr;
			outRank = fIterators.back().fRank;
		}
		
		uint32 v;
		uint8 r;

		if (fIterators.back().fIter->Next(v, r, false))
		{
			fIterators.back().fDocNr = v + fIterators.back().fOffset;
			fIterators.back().fRank = r;
			push_heap(fIterators.begin(), fIterators.end(), greater<CSubIter>());
		}
		else
		{
			delete fIterators.back().fIter;
			fIterators.erase(fIterators.begin() + fIterators.size() - 1);
		}
	}
	
	return result;
}

uint32 CDbJoinedIterator::Count() const
{
	return fCount;
}

uint32 CDbJoinedIterator::Read() const
{
	return fRead;
}

void CDbJoinedIterator::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		(*i).fIter->PrintHierarchy(inLevel + 1);
}

