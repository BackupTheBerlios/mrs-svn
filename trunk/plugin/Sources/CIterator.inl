/*	$Id$
	Copyright hekkelman
	Created Friday February 10 2006 09:24:21
*/

#ifndef CITERATOR_INL
#define CITERATOR_INL

#include "HStdCMath.h"

template<typename T>
CDbDocIteratorBaseT<T>::CDbDocIteratorBaseT(HStreamBase& inData,
		int64 inOffset, int64 inMax)
	: fBits(inData, inOffset)
	, fIter(fBits, inMax)
{
	using namespace std;

//	float s = 1000 * static_cast<float>(fIter.Count()) / inMax;
//	fCorrectionFactor = pow(1 / (s + 1), 0.3f);
	fIDFCorrectionFactor = log(1.0f + inMax / fIter.Count());
}

template<typename T>
inline
bool CDbDocIteratorBaseT<T>::Next(uint32& ioDoc, bool inSkip)
{
	bool done = false;

	while (not done and fIter.Next())
	{
		uint32 v = fIter.Value();

		if (v > ioDoc or v == 0 or not inSkip)
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
		uint32 v = fIter.Value();
		uint32 r = fIter.Weight();

		if (v > ioDoc or v == 0 or not inSkip)
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
