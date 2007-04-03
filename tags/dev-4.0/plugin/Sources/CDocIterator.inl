/*	$Id$
	Copyright hekkelman
	Created Friday February 10 2006 09:24:21
*/

#ifndef CITERATOR_INL
#define CITERATOR_INL

#include <cmath>
#include <iterator>

template<typename T, uint32 K>
CDbDocIteratorBaseT<T,K>::CDbDocIteratorBaseT(HStreamBase& inData,
		int64 inOffset, int64 inMax, uint32 inDelta)
	: fBits(inData, inOffset)
	, fIter(fBits, inMax)
	, fDelta(inDelta)
{
	using namespace std;

	fIDFCorrectionFactor = log(1.0 + static_cast<double>(inMax) / fIter.Count());
}

template<typename T, uint32 K>
CDbDocIteratorBaseT<T,K>::CDbDocIteratorBaseT(const char* inData, int64 inMax, uint32 inDelta)
	: fBits(inData, std::numeric_limits<uint32>::max())
	, fIter(fBits, inMax)
	, fDelta(inDelta)
{
	using namespace std;

	fIDFCorrectionFactor = log(1.0 + static_cast<double>(inMax) / fIter.Count());
}

template<typename T, uint32 K>
inline
bool CDbDocIteratorBaseT<T,K>::Next(uint32& ioDoc, bool inSkip)
{
	bool done = false;

	while (not done and fIter.Next())
	{
		uint32 v = fDelta + fIter.Value();

		if (v > ioDoc or v == fDelta or not inSkip)
		{
			ioDoc = v;
			done = true;
		}
	}

	return done;
}

template<typename T, uint32 K>
inline
bool CDbDocIteratorBaseT<T,K>::Next(uint32& ioDoc, uint8& ioWeight, bool inSkip)
{
	bool done = false;

	while (not done and fIter.Next())
	{
		uint32 v = fDelta + fIter.Value();
		uint32 r = fIter.Weight();

		if (v > ioDoc or v == fDelta or not inSkip)
		{
			ioDoc = v;
			ioWeight = r;
			done = true;
		}
	}

	return done;
}

template<typename T, uint32 K>
uint32 CDbDocIteratorBaseT<T,K>::Read() const
{
	return fIter.Read();
}

template<typename T, uint32 K>
uint32 CDbDocIteratorBaseT<T,K>::Count() const
{
	return fIter.Count();
}

template<typename T, uint32 K>
uint32 CDbDocIteratorBaseT<T,K>::Weight() const
{
	return fIter.Weight();
}

///////////////////////////////////////////////////////////////////////
//
//
//

template<uint32 K>
CDbIDLDocIteratorBaseT<K>::CDbIDLDocIteratorBaseT(
	HStreamBase& inData, int64 inOffset, int64 inMax, uint32 inDelta)
	: CDbDocIteratorBaseT<uint32,K>(inData, inOffset, inMax, inDelta)
{
}

template<uint32 K>
bool CDbIDLDocIteratorBaseT<K>::Next(uint32& ioDoc, bool inSkip)
{
	std::vector<uint32> t;
	return Next(ioDoc, t, inSkip);
}

template<uint32 K>
bool CDbIDLDocIteratorBaseT<K>::Next(uint32& ioDoc, uint8& ioRank, bool inSkip)
{
	bool result = false;
	std::vector<uint32> t;

	if (Next(ioDoc, t, inSkip))
	{
		ioRank = CDbDocIteratorBaseT<uint32,K>::Weight();
		result = true;
	}
	
	return result;
}

template<uint32 K>
bool CDbIDLDocIteratorBaseT<K>::Next(uint32& ioDoc, std::vector<uint32>& outLoc, bool inSkip)
{
	bool result = false;
	uint32 next = ioDoc;
	
	while (result == false and CDbDocIteratorBaseT<uint32,K>::Next(next, false))
	{
		DecompressArray<std::vector<uint32>,K>(
			CDbDocIteratorBaseT<uint32,K>::fBits, outLoc, kMaxInDocumentLocation);

		if (next >= ioDoc or inSkip == false)
		{
			result = true;
			ioDoc = next;
		}
	}

	return result;
}

template<uint32 K>
bool CDbIDLDocIteratorBaseT<K>::Next(uint32& ioDoc, COBitStream& outIDLCopy, bool inSkip)
{
	bool result = false;
	uint32 next = ioDoc;
	
	while (result == false and CDbDocIteratorBaseT<uint32,K>::Next(next, false))
	{
		CopyArray<uint32,K>(
			CDbDocIteratorBaseT<uint32,K>::fBits, outIDLCopy, kMaxInDocumentLocation);

		if (next >= ioDoc or inSkip == false)
		{
			result = true;
			ioDoc = next;
		}
	}

	return result;
}

///////////////////////////////////////////////////////////////////////
//
//	CDbPhraseDocIterator, very similar to CDocIntersectionIterator
//

template<uint32 K>
CDbPhraseDocIterator<K>::CDbPhraseDocIterator(std::vector<CDocIterator*>& inIters)
	: fCount(kInvalidDocID)
	, fRead(0)
{
	for (std::vector<CDocIterator*>::iterator i = inIters.begin(); i != inIters.end(); ++i)
	{
		CDbIDLDocIterator* iter = dynamic_cast<CDbIDLDocIterator*>(*i);
		
		if (iter == nil)
			THROW(("Invalid type or nil passed in CDbPhraseDocIterator constructor"));

		if ((*i)->Count() < fCount)
			fCount = (*i)->Count();
		
		CSubIter v;
		v.fIter = iter;
		v.fIndex = i - inIters.begin();
		if (not iter->Next(v.fValue, v.fIDL, false))
			v.fValue = kInvalidDocID;
		
		fIterators.push_back(v);
	}
	
	sort(fIterators.begin(), fIterators.end());
}

template<uint32 K>
CDbPhraseDocIterator<K>::~CDbPhraseDocIterator()
{
	for (CSubIterIterator i = fIterators.begin(); i != fIterators.end(); ++i)
		delete i->fIter;
}

template<uint32 K>
bool CDbPhraseDocIterator<K>::Next(uint32& ioValue, bool inSkip)
{
	bool result = false;
	
	while (fIterators.size() > 0 and not result)
	{
		CSubIterIterator i;

		result = true;
		uint32 next = fIterators.front().fValue;
		for (i = fIterators.begin() + 1; i != fIterators.end(); ++i)
		{
			result = result and i->fValue == next;
			if (i->fValue > next)
				next = i->fValue - 1;
		}
		
		if (result /*and inSkip*/ and next < ioValue)
			result = false;
		else
			ioValue = next;
		
		if (result)
		{
			// now check the IDL vectors

			// first subtract the 'in phrase index' to normalise phrase locations
			for (i = fIterators.begin(); i != fIterators.end(); ++i)
			{
				if (i->fIndex == 0)
					continue;
				
				std::transform(i->fIDL.begin(), i->fIDL.end(), i->fIDL.begin(),
					std::bind2nd(std::minus<uint32>(), i->fIndex));
			}
			
			fIDLCache1.clear();
			std::copy(fIterators.front().fIDL.begin(), fIterators.front().fIDL.end(),
				std::back_inserter(fIDLCache1));
			
			for (i = fIterators.begin() + 1; i != fIterators.end(); ++i)
			{
				fIDLCache2.clear();

				std::set_intersection(fIDLCache1.begin(), fIDLCache1.end(),
					i->fIDL.begin(), i->fIDL.end(),
					std::back_inserter(fIDLCache2));
				
				if (fIDLCache2.size() == 0)	// no adjacent words found
				{
					result = false;
					break;
				}
				
				swap(fIDLCache1, fIDLCache2);
			}			
		}

		for (i = fIterators.begin(); i != fIterators.end(); ++i)
		{
			if (i->fValue <= next)
			{
				uint32 v = next;
				if (not i->fIter->Next(v, i->fIDL, true))
				{
					for (CSubIterIterator j = fIterators.begin(); j != fIterators.end(); ++j)
						delete j->fIter;
					fIterators.erase(fIterators.begin(), fIterators.end());
					break;
				}
				else
					i->fValue = v;
			}
		}
		
		if (fIterators.size())
			sort(fIterators.begin(), fIterators.end());
	}

	if (result)
		++fRead;
	
	return result;
}

template<uint32 K>
uint32 CDbPhraseDocIterator<K>::Read() const
{
	return fRead;
}

template<uint32 K>
uint32 CDbPhraseDocIterator<K>::Count() const
{
	return fCount;
}

template<uint32 K>
void CDbPhraseDocIterator<K>::PrintHierarchy(int inLevel) const
{
	CDocIterator::PrintHierarchy(inLevel);
	
	typename std::vector<CSubIter>::const_iterator i;
	for (i = fIterators.begin(); i != fIterators.end(); ++i)
		i->fIter->PrintHierarchy(inLevel + 1);
}

///////////////////////////////////////////////////////////////////////
//
//	factory
//

inline
CDbDocIteratorBase*
CreateDbDocIterator(uint32 inKind, HStreamBase& inData, int64 inOffset,
	int64 inMax, uint32 inDelta, bool inContainsIDL)
{
	switch (inKind)
	{
		case kAC_GolombCode:
			if (inContainsIDL)
				return new CDbIDLDocIteratorGC(inData, inOffset, inMax, inDelta);
			else
				return new CDbDocIteratorGC(inData, inOffset, inMax, inDelta);

		case kAC_SelectorCodeV1:
			if (inContainsIDL)
				return new CDbIDLDocIteratorSC1(inData, inOffset, inMax, inDelta);
			else
				return new CDbDocIteratorSC1(inData, inOffset, inMax, inDelta);
		
		case kAC_SelectorCodeV2:
			if (inContainsIDL)
				return new CDbIDLDocIteratorSC2(inData, inOffset, inMax, inDelta);
			else
				return new CDbDocIteratorSC2(inData, inOffset, inMax, inDelta);
		
		default:
			THROW(("Unsupported array compression kind: %4.4s", &inKind));
	}
}

inline
CDbDocIteratorBase*
CreateDbDocWeightIterator(uint32 inKind, HStreamBase& inData, int64 inOffset, int64 inMax, uint32 inDelta = 0)
{
	switch (inKind)
	{
		case kAC_GolombCode:
			return new CDbDocWeightIteratorGC(inData, inOffset, inMax, inDelta);

		case kAC_SelectorCodeV1:
			return new CDbDocWeightIteratorSC1(inData, inOffset, inMax, inDelta);
		
		case kAC_SelectorCodeV2:
			return new CDbDocWeightIteratorSC2(inData, inOffset, inMax, inDelta);
		
		default:
			THROW(("Unsupported array compression kind: %4.4s", &inKind));
	}
}

#endif // CITERATOR_INL
