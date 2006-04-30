/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Monday December 09 2002 08:24:49
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
 
#ifndef CINDEXPAGE_H
#define CINDEXPAGE_H

#include <string>
#include <vector>
#include <stack>
#include <algorithm>

#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/if.hpp>

#include "CIterator.h"

class HStreamBase;

extern const uint32 kMaxKeySize;

// ---------------------------------------------------------------------------
// 
// First define the traits for comparing key types.
// 

int CompareKeyString(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB);
int CompareKeyNumber(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB);

template<uint32 INDEX_TYPE>
struct CIndexTraitsComp
{
	enum { uses_strcmp = 1 };
	
	int Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const
		{ return CompareKeyString(inA, inLengthA, inB, inLengthB); }
};

template<>
struct CIndexTraitsComp<kNumberIndex>
{
	enum { uses_strcmp = 0 };

	int Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const
		{ return CompareKeyNumber(inA, inLengthA, inB, inLengthB); }
};

template<uint32 INDEX_TYPE>
struct CIndexTraits
{
	typedef CIndexTraitsComp<INDEX_TYPE>		Comparator;
	Comparator									comp;
	
	int Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const
		{ return comp.Compare(inA, inLengthA, inB, inLengthB); }

	struct less : std::binary_function<const std::string&, const std::string&, bool>
	{
		Comparator								comp;
		
		bool operator()(const std::string& x, const std::string& y) const
			{ return comp.Compare(x.c_str(), x.length(), y.c_str(), y.length()) < 0; }
	};
};

typedef CIndexTraits<kTextIndex>::less		CTextLess;
typedef CIndexTraits<kDateIndex>::less		CDateLess;
typedef CIndexTraits<kValueIndex>::less		CValueLess;
typedef CIndexTraits<kNumberIndex>::less	CNumberLess;
typedef CIndexTraits<kWeightedIndex>::less	CWeightedLess;

// ---------------------------------------------------------------------------

class CIndex
{
  public:

						// normal constructor for an exisiting index on disk
						CIndex(uint32 inIndexKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot);
						~CIndex();
	
						// static factories to create new indices on disk
	static CIndex*		Create(uint32 inIndexKind, HStreamBase& inFile);

						// CreateFromIterator creates a compacted tree from sorted data
	static CIndex*		CreateFromIterator(uint32 inIndexKind, CIteratorBase& inData, HStreamBase& inFile);

						// access data
	bool				GetValue(const std::string& inKey, uint32& outValue) const;
	void				GetValuesForPattern(const std::string& inKey, std::vector<uint32>& outValues);

						// return the range [inLowerBound, inUpperBound)
	void				GetValuesForRange(const char* inLowerBound, const char* inUpperBound,
							 std::vector<uint32>& outValues);

						// return the values when compared to key and operator
	void				GetValuesForOperator(const std::string& inKey, CQueryOperator inOperator,
							 std::vector<uint32>& outValues);
	
	uint32				GetRoot() const;
	uint32				GetCount() const;
	uint32				GetKind() const;

#if P_DEBUG
	void				Dump() const;
#endif

	class iterator : public boost::iterator_facade<iterator,
		const std::pair<std::string,uint32>, boost::bidirectional_traversal_tag>
	{
	  public:
						iterator();
						iterator(const iterator& inOther);
		iterator&		operator=(const iterator& inOther);
		
	  private:
		friend class boost::iterator_core_access;
		friend class CIndex;
		friend struct CIndexImp;

						iterator(HStreamBase& inFile, int64 inOffset,
							uint32 inPage, uint32 inPageIndex);
	
		void			increment();
		void			decrement();
		bool			equal(const iterator& inOther) const;
		reference		dereference() const;
		
		HStreamBase*	fFile;
		int64			fBaseOffset;
		uint32			fPage;
		uint32			fPageIndex;
		std::pair<std::string,uint32>
						fCurrent;
	};

	iterator			begin();
	iterator			end();

	iterator			find(const std::string& inKey);
	iterator			lower_bound(const std::string& inKey);
	iterator			upper_bound(const std::string& inKey);

						// visitor

	struct VisitorBase
	{
		virtual			~VisitorBase() {}
		virtual void	Visit(const std::string& inKey, uint32& ioValue) = 0;
	};
	
	template <class T>
	struct Visitor : public VisitorBase
	{
		typedef void	(T::*VisitProc)(const std::string& inKey, uint32& ioValue);
		
						Visitor(T* inObject, VisitProc inMethod)
							: fObj(inObject)
							, fMethod(inMethod) {}
		
		virtual void	Visit(const std::string& inKey, uint32& ioValue)
							{ (fObj->*fMethod)(inKey, ioValue); }
	  private:
		T*				fObj;
		VisitProc		fMethod;
	};
	
	template <class T>
	void				Visit(T* inObject, typename Visitor<T>::VisitProc inMethod)
							{ Visitor<T> v(inObject, inMethod); Visit(v); }
	
  private:
						CIndex(const CIndex&);
	CIndex&				operator=(CIndex&);

	void				Visit(VisitorBase& inVisitor);

	struct CIndexImp*	fImpl;
};

#endif // CINDEXPAGE_H
