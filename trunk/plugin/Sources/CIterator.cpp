/*	$Id: CIterator.cpp 30 2006-04-30 17:36:03Z maarten $
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

#include <limits>
//#include "HUtils.h"

#include "CIterator.h"

using namespace std;

CIteratorBase::CIteratorBase()
{
}

CIteratorBase::CIteratorBase(const CIteratorBase&)
{
}

CIteratorBase::~CIteratorBase()
{
}
					
CIteratorBase& CIteratorBase::operator=(const CIteratorBase&)
{
	return *this;
}

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
			else
				delete *i;
		}
	}
	
	make_heap(fIterators.begin(), fIterators.end());
}

CStrUnionIterator::~CStrUnionIterator()
{
	for (vector<CSubIter>::iterator i = fIterators.begin(); i != fIterators.end(); ++i)
		delete i->fIter;
}

bool CStrUnionIterator::Next(string& outString, int64& outValue)
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
		int64 v = outValue;
		
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

CJoinedIterator::CJoinedIterator()
{
}

CJoinedIterator::CJoinedIterator(vector<CIteratorBase*>& inIters)
{
	vector<CIteratorBase*>::iterator i;
	for (i = inIters.begin(); i != inIters.end(); ++i)
	{
		CElement e;
		
		e.fIter = *i;
		
		if ((*i)->Next(e.fSValue, e.fNValue))
			fItems.push_back(e);
		else
			delete *i;
	}
	
	make_heap(fItems.begin(), fItems.end());
}

CJoinedIterator::~CJoinedIterator()
{
	CElementList::iterator e;
	for (e = fItems.begin(); e != fItems.end(); ++e)
		delete (*e).fIter;
}

void CJoinedIterator::Append(CIteratorBase* inIter)
{
	CElement e;
	
	e.fIter = inIter;
	
	if (inIter->Next(e.fSValue, e.fNValue))
	{
		fItems.push_back(e);
		make_heap(fItems.begin(), fItems.end());
	}
	else
		delete inIter;
}
	
bool CJoinedIterator::Next(string& outString, int64& outValue)
{
	bool result = false;
	
	if (fItems.size() > 0)
	{
		pop_heap(fItems.begin(), fItems.end());
		
		outString = fItems.back().fSValue;
		outValue = fItems.back().fNValue;
		result = true;
		
		if (fItems.back().fIter->Next(fItems.back().fSValue, fItems.back().fNValue))
			push_heap(fItems.begin(), fItems.end());
		else
		{
			delete fItems.back().fIter;
			fItems.erase(fItems.end() - 1);
		}
	}

	return result;
}
