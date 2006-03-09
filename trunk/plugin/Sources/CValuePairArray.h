/*	$Id$
	Copyright hekkelman
	Created Thursday February 09 2006 09:36:27
*/

#ifndef CVALUEPAIRARRAY_H
#define CVALUEPAIRARRAY_H

#if P_GNU
#include "CBitStream.h"
#else
class COBitStream;
class CIBitStream;
#endif

#include <boost/type_traits/is_pod.hpp>

//	Rationale:
//
//	We want to concentrate the code for writing golomb code compressed arrays
//	into one single area. We also support both the simple compressed document
//	id vectors as well as attributed document id vectors (containing e.g. document
//	id and weight).
//	For simplicity attributes are consided to be uint8 values in the range
//	0 < v <= kMaxWeight
//	For attributed vectors we sort the vectors on attribute value first, and
//	on document ID next. This allows us to store the document ID's with the
//	same attribute values as a regular compressed array. The attribute value
//	is stored before these arrays as differences from the previous attribute
//	value.

namespace CValuePairCompression
{

template<typename V, typename W>
struct WeightGreater
{
	bool			operator()(const std::pair<V,W>& inA, const std::pair<V,W>& inB) const
						{ return inA.second > inB.second or (inA.second == inB.second and inA.first < inB.first); }
};

template<typename T>
void CompressSimpleArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax)
{
	uint32 cnt = inArray.size();
	int32 b = CalculateB(inMax, cnt);
	
	int32 n = 0, g = 1;
	while (g < b)
	{
		++n;
		g <<= 1;
	}
	g -= b;

	WriteGamma(inBits, cnt);
	
	int64 lv = -1;	// we store delta's and our arrays can start at zero...
	
	typedef typename std::vector<T>::iterator iterator;
	
	for (iterator i = inArray.begin(); i != inArray.end(); ++i)
	{
		// write the value
		
		int32 d = static_cast<int32>(*i - lv);
		assert(d > 0);
		
		int32 q = (d - 1) / b;
		int32 r = d - q * b - 1;
		
		while (q-- > 0)
			inBits << 1;
		inBits << 0;
		
		if (b > 1)
		{
			if (r < g)
			{
				for (int t = 1 << (n - 2); t != 0; t >>= 1)
					inBits << ((r & t) != 0);
			}		
			else
			{
				r += g;
				for (int t = 1 << (n - 1); t != 0; t >>= 1)
					inBits << ((r & t) != 0);
			}
		}
		
		lv = *i;
	}
}

template<typename T>
struct ValuePairTraitsPOD
{
	typedef				std::vector<T>					vector_type;
	typedef typename	vector_type::iterator			iterator;
	typedef typename	vector_type::const_iterator		const_iterator;

	typedef 			T								value_type;
	typedef 			void							rank_type;

	static void			CompressArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax);
};

template<typename T>
void ValuePairTraitsPOD<T>::CompressArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax)
{
	CompressSimpleArray<T>(inBits, inArray, inMax);
}

template<typename T>
struct ValuePairTraitsPair
{
	typedef				std::vector<T>					vector_type;
	typedef typename	vector_type::iterator			iterator;
	typedef typename	vector_type::const_iterator		const_iterator;

	typedef typename	T::first_type					value_type;
	typedef typename	T::second_type					rank_type;

	static void			CompressArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax);
};

template<typename T>
void ValuePairTraitsPair<T>::CompressArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax)
{
	WriteGamma(inBits, inArray.size());

	sort(inArray.begin(), inArray.end(), WeightGreater<value_type,rank_type>());
	
	std::vector<value_type> values;
	values.reserve(inArray.size());
	
	const_iterator v = inArray.begin();
	
	uint32 lastWeight = inArray.front().second;
	WriteGamma(inBits, lastWeight);
	
	while (v != inArray.end())
	{
		uint32 w = v->second;
		
		while (v != inArray.end() and v->second == w)
		{
			values.push_back(v->first);
			++v;
		}
		
		if (values.size())
		{
			uint8 d = lastWeight - w;
			if (d > 0)	// skip the first since it has been written already
				WriteGamma(inBits, d);
			CompressSimpleArray(inBits, values, inMax);
			values.clear();
			lastWeight = w;
		}
	}
}

// an iterator type for compressed value/rank pairs

template<typename T>
class IteratorBase
{
  public:
					IteratorBase(CIBitStream& inData, int64 inMax);
	virtual			~IteratorBase() {}

	virtual bool	Next();
	
	T				Value() const					{ return static_cast<T>(fValue); }
	virtual uint32	Weight() const					{ return 1; }
	
	virtual uint32	Count() const					{ return fCount; }
	virtual uint32	Read() const					{ return fRead; }

  protected:
					IteratorBase(CIBitStream& inData, int64 inMax, bool);

					IteratorBase(const IteratorBase& inOther);
	IteratorBase&		operator=(const IteratorBase& inOther);

	void			Reset();

	CIBitStream*	fBits;
	uint32			fCount;
	uint32			fRead;
	int32			b, n, g;

	int64			fValue;
	int64			fMax;
};

template<typename T>
IteratorBase<T>::IteratorBase(CIBitStream& inData, int64 inMax)
	: fBits(&inData)
	, fRead(0)
	, fValue(-1)
	, fMax(inMax)
{
	Reset();
}

template<typename T>
IteratorBase<T>::IteratorBase(CIBitStream& inData, int64 inMax, bool)
	: fBits(&inData)
	, fRead(0)
	, fValue(-1)
	, fMax(inMax)
{
}

template<typename T>
void IteratorBase<T>::Reset()
{
	fValue = -1;
	fCount = ReadGamma(*fBits);
	fRead = 0;
	b = CalculateB(fMax, fCount);
	n = 0;
	g = 1;
	
	while (g < b)
	{
		++n;
		g <<= 1;
	}
	g -= b;
}

template<typename T>
bool IteratorBase<T>::Next()
{
	bool done = false;

	if (fRead < fCount)
	{
		int32 q = 0;
		int32 r = 0;
		
		if (fBits->next_bit())
		{
			q = 1;
			while (fBits->next_bit())
				++q;
		}
		
		if (b > 1)
		{
			for (int e = 0; e < n - 1; ++e)
				r = (r << 1) | fBits->next_bit();
			
			if (r >= g)
			{
				r = (r << 1) | fBits->next_bit();
				r -= g;
			}
		}
		
		int32 d = r + q * b + 1;
		
		fValue += d;
		++fRead;
		
		done = true;
	}

	return done;
}

template<typename T>
class VRIterator : public IteratorBase<typename ValuePairTraitsPair<T>::value_type>
{
	typedef IteratorBase<typename ValuePairTraitsPair<T>::value_type>	base_type;

	typedef				ValuePairTraitsPair<T>	traits;
	typedef typename	traits::value_type		value_type;
	typedef typename	traits::rank_type		rank_type;
	
  public:
					VRIterator(CIBitStream& inData, int64 inMax)
						: base_type(inData, inMax, false)
					{
						fTotalCount = ReadGamma(inData);
						fWeight = ReadGamma(inData);
						fTotalRead = 0;
						base_type::Reset();
					}

	virtual bool	Next();
	
	virtual uint32	Weight() const					{ return fWeight; }

	virtual uint32	Read() const					{ return fTotalRead; }
	virtual uint32	Count() const					{ return fTotalCount; }

  private:

	//				VRIterator(const VRIterator& inOther);
	//VRIterator&		operator=(const VRIterator& inOther);

	void			Restart();

	uint32			fTotalCount;
	uint32			fTotalRead;
	uint32			fWeight;
};

template<typename T>
void VRIterator<T>::Restart()
{
	fWeight -= ReadGamma(*base_type::fBits);
	
	assert(fWeight <= kMaxWeight);
	assert(fWeight > 0);

	if (fWeight > 0)
		base_type::Reset();
	else
		base_type::fCount = 0;

	assert(fCount <= fTotalCount - fTotalRead);
}

template<typename T>
bool VRIterator<T>::Next()
{
	bool done = false;
	
	while (not done and fWeight > 0 and fTotalRead < fTotalCount)
	{
		done = base_type::Next();

		if (not done)
			Restart();
		else
			++fTotalRead;
	}
	
	return done;
}

template<typename T, bool>
struct ValuePairTraitsTypeFactoryBase
{
	typedef	ValuePairTraitsPOD<T>	type;
	typedef IteratorBase<T>			iterator;
};

template<typename T>
struct ValuePairTraitsTypeFactoryBase<T, false>
{
	typedef	ValuePairTraitsPair<T>	type;
	typedef VRIterator<T>			iterator;
};

template<typename T>
struct ValuePairTraitsTypeFactory : public ValuePairTraitsTypeFactoryBase<T, boost::is_pod<T>::value>
{
};

}

template<typename T>
void CompressArray(COBitStream& inBits, T& inArray, int64 inMax)
{
	using namespace		CValuePairCompression;

	typedef typename	T::value_type										vector_value_type;
	typedef typename	ValuePairTraitsTypeFactory<vector_value_type>::type	traits;

	traits::CompressArray(inBits, inArray, inMax);
}

#endif // CVALUEPAIRARRAY_H
