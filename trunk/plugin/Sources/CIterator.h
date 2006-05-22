/*	$Id: CIterator.h 30 2006-04-30 17:36:03Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday March 14 2004 13:51:23
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
 
#ifndef CITERATOR_H
#define CITERATOR_H

#include <string>
#include <vector>

#include "HError.h"

#include "CIndex.h"

class CIteratorBase
{
  public:

	virtual			~CIteratorBase();

	virtual bool	Next(std::string& outString, int64& outValue) = 0;
	virtual bool	Prev(std::string& outKey, int64& outValue) { return false; }

  protected:
	
					CIteratorBase();
					CIteratorBase(const CIteratorBase&);
					
	CIteratorBase&	operator=(const CIteratorBase&);
};

// The next iterator is a wrapper around an STL container
// containing pair's of string and uint32

template<class T>
class CIteratorWrapper : public CIteratorBase
{
	typedef typename T::iterator	iterator;
	
  public:
						CIteratorWrapper(T& inContainer, int32 inDelta = 0);

	virtual bool		Next(std::string& outString, int64& outValue);
	virtual bool		Prev(std::string& outKey, int64& outValue);

  protected:
	iterator			fBegin, fEnd, fCurrent;
	int32				fDelta;
};

template<class T>
CIteratorWrapper<T>::CIteratorWrapper(T& inContainer, int32 inDelta)
	: fBegin(inContainer.begin())
	, fEnd(inContainer.end())
	, fCurrent(fBegin)
	, fDelta(inDelta)
{
}

template<class T>
bool CIteratorWrapper<T>::Next(std::string& outKey, int64& outValue)
{
	bool result = false;
	if (fCurrent != fEnd)
	{
		outKey = fCurrent->first;
		outValue = fCurrent->second + fDelta;

		++fCurrent;

		result = true;
	}
	return result;
}

template<class T>
bool CIteratorWrapper<T>::Prev(std::string& outKey, int64& outValue)
{
	bool result = false;
	if (fCurrent != fBegin)
	{
		--fCurrent;

		outKey = fCurrent->first;
		outValue = fCurrent->second + fDelta;

		result = true;
	}
	return result;
}

class CStrUnionIterator : public CIteratorBase
{
  public:
					CStrUnionIterator(std::vector<CIteratorBase*>& inIters);
	virtual			~CStrUnionIterator();
	
	virtual bool	Next(std::string& outString, int64& outValue);

  private:

	struct CSubIter
	{
		std::string		fKey;
		int64			fValue;
		CIteratorBase*	fIter;
		
		bool operator<(const CSubIter& inOther) const
				{ return fKey > inOther.fKey; }
	};

	std::vector<CSubIter>	fIterators;
};

class CJoinedIteratorBase : public CIteratorBase
{
  public:

	virtual void	Append(CIteratorBase* inIter) = 0;

	static CJoinedIteratorBase*	Create(uint32 inKind);
};

template<uint32 INDEX_TYPE>
class CJoinedIterator : public CJoinedIteratorBase
{
	typedef typename CIndexTraits<INDEX_TYPE>::Comparator			Comparator;

  public:
					CJoinedIterator();
					~CJoinedIterator();
	
	virtual void	Append(CIteratorBase* inIter);

	virtual bool	Next(std::string& outString, int64& outValue);

  private:
	
	struct CElement
	{
		typedef typename CJoinedIterator<INDEX_TYPE>::Comparator	Comparator;
		
		std::string		fSValue;
		int64			fNValue;
		CIteratorBase*	fIter;
		
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
void CJoinedIterator<INDEX_TYPE>::Append(CIteratorBase* inIter)
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

template<uint32 INDEX_TYPE>
bool CJoinedIterator<INDEX_TYPE>::Next(std::string& outString, int64& outValue)
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

#endif // CITERATOR_H
