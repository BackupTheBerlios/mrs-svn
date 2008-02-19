/*	$Id$
	Copyright hekkelman
	Created Friday February 10 2006 09:24:21
*/

#ifndef CITERATOR_INL
#define CITERATOR_INL

#include <cmath>

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
	: fBits(inData)
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

inline
CDbDocIteratorBase*
CreateDbDocIterator(uint32 inKind, HStreamBase& inData, int64 inOffset, int64 inMax, uint32 inDelta = 0)
{
	switch (inKind)
	{
		case kAC_GolombCode:
			return new CDbDocIteratorGC(inData, inOffset, inMax, inDelta);

		case kAC_SelectorCodeV1:
			return new CDbDocIteratorSC1(inData, inOffset, inMax, inDelta);
		
		case kAC_SelectorCodeV2:
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
