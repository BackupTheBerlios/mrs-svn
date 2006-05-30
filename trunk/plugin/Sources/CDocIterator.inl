/*	$Id$
	Copyright hekkelman
	Created Friday February 10 2006 09:24:21
*/

#ifndef CITERATOR_INL
#define CITERATOR_INL

#include <cmath>

template<typename T>
CDbDocIteratorBaseT<T>::CDbDocIteratorBaseT(HStreamBase& inData,
		int64 inOffset, int64 inMax, uint32 inDelta)
	: fBits(inData, inOffset)
	, fIter(fBits, inMax)
	, fDelta(inDelta)
{
	using namespace std;

	fIDFCorrectionFactor = log(1.0 + static_cast<double>(inMax) / fIter.Count());
}

template<typename T>
inline
bool CDbDocIteratorBaseT<T>::Next(uint32& ioDoc, bool inSkip)
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

template<typename T>
inline
bool CDbDocIteratorBaseT<T>::Next(uint32& ioDoc, uint8& ioWeight, bool inSkip)
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

template<typename T>
uint32 CDbDocIteratorBaseT<T>::Read() const
{
	return fIter.Read();
}

template<typename T>
uint32 CDbDocIteratorBaseT<T>::Count() const
{
	return fIter.Count();
}

template<typename T>
uint32 CDbDocIteratorBaseT<T>::Weight() const
{
	return fIter.Weight();
}

#endif // CITERATOR_INL
