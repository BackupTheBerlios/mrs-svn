/*	$Id$
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

#include "CValuePairArray.h"

class HStreamBase;
class CIBitStream;
class CDatabankBase;

class CIteratorBase
{
  public:

	virtual			~CIteratorBase();

	virtual bool	Next(std::string& outString, uint32& outValue) = 0;
	virtual bool	Prev(std::string& outKey, uint32& outValue) { return false; }
//	virtual bool	Goto(std::string inKey) = 0;

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
						CIteratorWrapper(T& inContainer);

	virtual bool		Next(std::string& outString, uint32& outValue);
	virtual bool		Prev(std::string& outKey, uint32& outValue);

  protected:
	iterator			fBegin, fEnd, fCurrent;
};

template<class T>
CIteratorWrapper<T>::CIteratorWrapper(T& inContainer)
	: fBegin(inContainer.begin())
	, fEnd(inContainer.end())
	, fCurrent(fBegin)
{
}

template<class T>
bool CIteratorWrapper<T>::Next(std::string& outKey, uint32& outValue)
{
	bool result = false;
	if (fCurrent != fEnd)
	{
		std::pair<std::string,uint32> c = *fCurrent;

		++fCurrent;
		
		outKey = c.first;
		outValue = c.second;
		
		result = true;
	}
	return result;
}

template<class T>
bool CIteratorWrapper<T>::Prev(std::string& outKey, uint32& outValue)
{
	bool result = false;
	if (fCurrent != fBegin)
	{
		--fCurrent;

		std::pair<std::string,uint32> c = *fCurrent;
		
		outKey = c.first;
		outValue = c.second;
		
		result = true;
	}
	return result;
}

class CStrUnionIterator : public CIteratorBase
{
  public:
					CStrUnionIterator(std::vector<CIteratorBase*>& inIters);
	virtual			~CStrUnionIterator();
	
	virtual bool	Next(std::string& outString, uint32& outValue);

  private:

	struct CSubIter
	{
		std::string		fKey;
		uint32			fValue;
		CIteratorBase*	fIter;
		
		bool operator<(const CSubIter& inOther) const
				{ return fKey > inOther.fKey; }
	};

	std::vector<CSubIter>	fIterators;
//	uint32					fCount, fRead;
};

class CDocIterator
{
  public:
					CDocIterator();
	virtual			~CDocIterator();

	virtual bool	Next(uint32& ioDoc, bool inSkip) = 0;
	virtual uint32	Count() const = 0;
	virtual uint32	Read() const = 0;

	virtual void	PrintHierarchy(int inLevel) const;
};

class CDocDeltaIterator : public CDocIterator
{
  public:
					CDocDeltaIterator(CDocIterator* inOriginal,
						int32 inOffset = 0);
	virtual			~CDocDeltaIterator();
	
	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

	virtual void	PrintHierarchy(int inLevel) const;

  protected:
	CDocIterator*	fOriginal;
	int32			fOffset;
};

// the iterator for a simple one file databank

class CDbDocIteratorBase : public CDocIterator
{
  public:
	virtual bool	Next(uint32& ioDoc, uint8& ioRank, bool inSkip) = 0;

	virtual float	GetIDFCorrectionFactor() const			{ return fIDFCorrectionFactor; }

	virtual	uint32	Weight() const							{ return 1; }

  protected:
	float			fIDFCorrectionFactor;
};

template<typename T>
class CDbDocIteratorBaseT : public CDbDocIteratorBase
{
	typedef typename	CValuePairCompression::ValuePairTraitsTypeFactory<T>::type		traits;
	typedef typename	CValuePairCompression::ValuePairTraitsTypeFactory<T>::iterator	IterType;
	
  public:
					CDbDocIteratorBaseT(HStreamBase& inData,
						int64 inOffset, int64 inMax);

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual bool	Next(uint32& ioDoc, uint8& ioRank, bool inSkip);

	virtual	uint32	Weight() const;

	virtual uint32	Count() const;
	virtual uint32	Read() const;

  protected:
	CIBitStream		fBits;
	IterType		fIter;
};

typedef CDbDocIteratorBaseT<uint32>						CDbDocIterator;
typedef CDbDocIteratorBaseT<std::pair<uint32,uint8> >	CDbDocWeightIterator;

class CMergedDbDocIterator : public CDbDocIteratorBase
{
  public:
					CMergedDbDocIterator(CDbDocIteratorBase* inIterA, uint32 inDeltaA, uint32 inDbCountA,
						CDbDocIteratorBase* inIterB, uint32 inDeltaB, uint32 inDbCountB);

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual bool	Next(uint32& ioDoc, uint8& ioRank, bool inSkip);

	virtual uint32	Count() const						{ return fCount; }
	virtual uint32	Read() const						{ return fRead; }

  private:

	struct CSubIter
	{
		uint32				fDocNr;
		uint8				fRank;
		uint32				fDelta;
		CDbDocIteratorBase*	fIter;
		
		bool operator<(const CSubIter& inOther) const
				{ return fRank > inOther.fRank or (fRank == inOther.fRank and fDocNr + fDelta < inOther.fDocNr + inOther.fDelta); }

		bool operator>(const CSubIter& inOther) const
				{ return fRank < inOther.fRank or (fRank == inOther.fRank and fDocNr + fDelta > inOther.fDocNr + inOther.fDelta); }
	};

	std::vector<CSubIter>	fIterators;
	uint32					fCount;
	uint32					fRead;
};

class CDocNrIterator : public CDocIterator
{
  public:
					CDocNrIterator(uint32 inValue);
	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

  private:
	uint32			fValue;
};

class CDocUnionIterator : public CDocIterator
{
  public:
	static CDocIterator*	Create(std::vector<CDocIterator*>& inIters);
	static CDocIterator*	Create(CDocIterator* inFirst, CDocIterator* inSecond);

	virtual			~CDocUnionIterator();

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Read() const;
	virtual uint32	Count() const;

	virtual void	PrintHierarchy(int inLevel) const;

  private:
					CDocUnionIterator(std::vector<CDocIterator*>& inIters);
	
	struct CSubIter
	{
		uint32			fValue;
		CDocIterator*	fIter;
		
		bool operator<(const CSubIter& inOther) const
				{ return fValue > inOther.fValue; }
	};

	std::vector<CSubIter>	fIterators;
	uint32					fCount, fRead;
};

class CDocIntersectionIterator : public CDocIterator
{
  public:
	static CDocIterator*	Create(std::vector<CDocIterator*>& inIters);
	static CDocIterator*	Create(CDocIterator* inFirst, CDocIterator* inSecond);

	virtual			~CDocIntersectionIterator();

	virtual bool	Next(uint32& ioValue, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

	virtual void	PrintHierarchy(int inLevel) const;

  private:

					CDocIntersectionIterator(std::vector<CDocIterator*>& inIters);

	struct CSubIter
	{
		uint32				fValue;
		CDocIterator*		fIter;
		
		bool operator<(const CSubIter& inOther) const
				{ return fValue < inOther.fValue; }
	};

	std::vector<CSubIter>	fIterators;
	uint32					fCount, fRead;
};

class CDocNotIterator : public CDocIterator
{
  public:
					CDocNotIterator(CDocIterator* inIter, uint32 inMax);
	virtual			~CDocNotIterator();

	virtual bool	Next(uint32& ioValue, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

	virtual void	PrintHierarchy(int inLevel) const;

  private:
	CDocIterator*	fIter;
	uint32			fCur, fNext, fMax, fRead;
};

class CDocVectorIterator : public CDocIterator
{
  public:
	typedef std::vector<std::pair<uint32,float> > DocFreqVector;

					CDocVectorIterator(DocFreqVector* inDocs)
						: fDocs(inDocs)
						, fCur(0)
						, fRead(0) {}
					CDocVectorIterator(const DocFreqVector& inDocs);
	
	virtual bool	Next(uint32& ioDoc, float& outScore, bool inSkip);
	virtual bool	Next(uint32& ioValue, bool inSkip);
	virtual uint32	Count() const						{ return fDocs->size(); }
	virtual uint32	Read() const						{ return fRead; }

  private:
	std::auto_ptr<DocFreqVector>	fDocs;
	uint32			fCur, fRead;
};

class CDbAllDocIterator : public CDocIterator
{
  public:
					CDbAllDocIterator(uint32 inMaxDocNr);
	virtual			~CDbAllDocIterator();

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

  private:
	uint32			fDocNr;
	uint32			fMaxDocNr;
};

// cached iterator, fetch the first N documents to speed up boolean searches
class CDocCachedIterator : public CDocIterator
{
  public:
					CDocCachedIterator(CDocIterator* inIterator, uint32 inCache = 1000);

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;
	virtual void	PrintHierarchy(int inLevel) const;

  private:
	std::auto_ptr<CDocIterator>		fIter;
	std::vector<uint32>				fCache;
	uint32							fPointer;
	bool							fCacheContainsAll;
};

class CDbStringMatchIterator : public CDocIterator
{
  public:
					CDbStringMatchIterator(CDatabankBase& inDb, const std::vector<std::string>& inStringWords,
						CDocIterator* inBaseIterator);
	virtual			~CDbStringMatchIterator();

	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

  private:
	CDatabankBase&	fDb;
	std::vector<std::string>
					fStringWords;
	CDocIterator*	fIter;
};

class CDbJoinedIterator : public CDbDocIteratorBase
{
  public:
					CDbJoinedIterator();
	virtual			~CDbJoinedIterator();
	
	void			AddIterator(CDbDocIteratorBase* inIter, uint32 inCount);
	
	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual bool	Next(uint32& ioDoc, uint8& outRank, bool inSkip);
	virtual uint32	Count() const;
	virtual uint32	Read() const;

	virtual void	PrintHierarchy(int inLevel) const;

  protected:
	struct CSubIter
	{
		uint32				fOffset;
		uint32				fDocNr;
		uint8				fRank;
		CDbDocIteratorBase*	fIter;
		
		bool operator>(const CSubIter& inOther) const
				{ return fRank < inOther.fRank or (fRank == inOther.fRank and fDocNr > inOther.fDocNr); }
	};

	std::vector<CSubIter>	fIterators;
	uint32					fDocCount;
	uint32					fCount, fRead;
};

#include "CIterator.inl"

#endif // CITERATOR_H
