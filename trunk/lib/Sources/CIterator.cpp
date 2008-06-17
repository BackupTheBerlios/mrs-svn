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
#include <iostream>

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

template<uint32 INDEX_TYPE>
class CJoinedIterator : public CJoinedIteratorBase
{
	typedef typename CIndexTraits<INDEX_TYPE>::Comparator			Comparator;

  public:
					CJoinedIterator();
					~CJoinedIterator();
	
	virtual void	Append(CIteratorBase* inIter, uint32 inIndexNr);

	virtual bool	Next(std::string& outString, int64& outValue);
	virtual bool	Next(std::string& outString, std::vector<std::pair<uint32, int64> >& outValues);

  private:
	
	struct CElement
	{
		typedef typename CJoinedIterator<INDEX_TYPE>::Comparator	Comparator;
		
		int64			fNValue;
		std::string		fSValue;
		CIteratorBase*	fIter;
		uint32			fIndexNr;
		
		bool operator<(const CElement& inOther) const
		{
			Comparator	comp;
			int d = comp.Compare(fSValue.c_str(), fSValue.length(),
				inOther.fSValue.c_str(), inOther.fSValue.length());
			return d > 0 or d == 0 and fNValue > inOther.fNValue;
		}
	};

	typedef std::vector<CElement>				CElementList;
	typedef typename CElementList::iterator		CElementListIterator;

	CElementList		fItems;
};


template<uint32 INDEX_TYPE>
CJoinedIterator<INDEX_TYPE>::CJoinedIterator()
{
}

template<uint32 INDEX_TYPE>
CJoinedIterator<INDEX_TYPE>::~CJoinedIterator()
{
	CElementListIterator e;
	for (e = fItems.begin(); e != fItems.end(); ++e)
		delete e->fIter;
}

template<uint32 INDEX_TYPE>
void CJoinedIterator<INDEX_TYPE>::Append(CIteratorBase* inIter, uint32 inIndexNr)
{
	CElement e;
	
	e.fIter = inIter;
	e.fIndexNr = inIndexNr;
	
	if (inIter->Next(e.fSValue, e.fNValue))
	{
		fItems.push_back(e);
		make_heap(fItems.begin(), fItems.end());
	}
	else
		delete inIter;
}

template<uint32 INDEX_TYPE>
bool CJoinedIterator<INDEX_TYPE>::Next(std::string& outString, int64& outValue)
{
	THROW(("Error, should not be called"));
}

template<uint32 INDEX_TYPE>
bool CJoinedIterator<INDEX_TYPE>::Next(std::string& outString,
	std::vector<std::pair<uint32, int64> >& outValues)
{
	bool result = false;
	
	Comparator comp;
	
	outValues.clear();
	
	if (fItems.size() > 0)
	{
		result = true;

		outString = fItems.front().fSValue;

		do
		{
			pop_heap(fItems.begin(), fItems.end());

			outValues.push_back(make_pair(fItems.back().fIndexNr, fItems.back().fNValue));
			
			if (fItems.back().fIter->Next(fItems.back().fSValue, fItems.back().fNValue))
				push_heap(fItems.begin(), fItems.end());
			else
			{
				delete fItems.back().fIter;
				fItems.erase(fItems.end() - 1);
			}
		}
		while (fItems.size() > 0 and
			comp.Compare(fItems.front().fSValue.c_str(), fItems.front().fSValue.length(),
					outString.c_str(), outString.length()) == 0);
	}

	return result;
}

CJoinedIteratorBase* CJoinedIteratorBase::Create(uint32 inKind)
{
	switch (inKind)
	{
		case kTextIndex:		return new CJoinedIterator<kTextIndex>();
		case kDateIndex:		return new CJoinedIterator<kDateIndex>();
		case kValueIndex:		return new CJoinedIterator<kValueIndex>();
		case kNumberIndex:		return new CJoinedIterator<kNumberIndex>();
		case kWeightedIndex:	return new CJoinedIterator<kWeightedIndex>();
		default:				THROW(("Unsupported index kind: %4.4s", &inKind));
	}
}

