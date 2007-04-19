/*	$Id: CValuePairArray.h 105 2006-06-14 06:22:25Z maarten $
	Copyright hekkelman
	Created Thursday February 09 2006 09:36:27
*/

#ifndef CVALUEPAIRARRAY_H
#define CVALUEPAIRARRAY_H

#if P_DEBUG
#include <iostream>
#endif

#if P_GNU
#include "CBitStream.h"
#else
class COBitStream;
class CIBitStream;
#endif

#include "HError.h"
#include <boost/type_traits/is_integral.hpp>

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
//
//	The original version of MRS used Golomb code compression solely. However,
//	a recent article by Vo Ngoc Anh and Alistair Moffat called 'Index compression
//	using Fixed Binary Codewords' showed that big improvements can be attained
//	using another compressions algorithm. The gain in decompression speed is
//	about 20% however, surprisingly the gain in compression ratio in the typical
//	bioinformatics databank results in MRS files that are typically 10 to 20%
//	smaller.

enum CArrayCompressionKind {
	kAC_GolombCode		= FOUR_CHAR_INLINE('golo'),
	kAC_SelectorCodeV1	= FOUR_CHAR_INLINE('sel1'),
	
		// found an error in the implementation and a potential performance improvement
	kAC_SelectorCodeV2	= FOUR_CHAR_INLINE('sel2'),
};

namespace CValuePairCompression
{

template<typename V, typename W>
struct WeightGreater
{
	bool			operator()(const std::pair<V,W>& inA, const std::pair<V,W>& inB) const
						{ return inA.second > inB.second or (inA.second == inB.second and inA.first < inB.first); }
};

struct CSelector
{
	int		databits;
	int		span;
};

const CSelector kSelectors[16] = {
	{  0, 1 },
	{ -3, 1 },
	{ -2, 1 }, { -2, 2 },
	{ -1, 1 }, { -1, 2 }, { -1, 4 },
	{  0, 1 }, {  0, 2 }, {  0, 4 },
	{  1, 1 }, {  1, 2 }, {  1, 4 },
	{  2, 1 }, {  2, 2 },
	{  3, 1 }
};

inline uint32 Select(uint32 inBitsNeeded[], uint32 inCount, int32 inWidth, int32 inMaxWidth,
	const CSelector inSelectors[])
{
	uint32 result = 0;
	int32 c = inBitsNeeded[0] - inMaxWidth;
	
	for (uint32 i = 1; i < 16; ++i)
	{
		if (inSelectors[i].span > inCount)
			continue;
		
		int32 w = inWidth + inSelectors[i].databits;
		
		if (w > inMaxWidth or w < 0)
			continue;
		
		bool fits = true;
		int32 waste = 0;
		
		switch (inSelectors[i].span)
		{
			case 4:
				fits = fits and inBitsNeeded[3] <= w;
				waste += w - inBitsNeeded[3];
			case 3:
				fits = fits and inBitsNeeded[2] <= w;
				waste += w - inBitsNeeded[2];
			case 2:
				fits = fits and inBitsNeeded[1] <= w;
				waste += w - inBitsNeeded[1];
			case 1:
				fits = fits and inBitsNeeded[0] <= w;
				waste += w - inBitsNeeded[0];
		}
		
		if (fits == false)
			continue;
		
		int32 n = (inSelectors[i].span - 1) * 4 - waste;
		
		if (n > c)
		{
			result = i;
			c = n;
		}
	}
	
	return result;
}

template<class T>
inline void Shift(T& ioIterator, int64& ioLast, uint32& outDelta, uint32& outWidth)
{
	int64 next = *ioIterator++;
	assert(next > ioLast);
	
	outDelta = next - ioLast - 1;
	ioLast = next;
	
	uint32 v = outDelta;
	outWidth = 0;
	while (v > 0)
	{
		v >>= 1;
		++outWidth;
	}
}

template<typename T>
void CompressSimpleArraySelector(COBitStream& inBits, std::vector<T>& inArray, int64 inMax)
{
	uint32 cnt = inArray.size();

	WriteGamma(inBits, cnt);

	int32 maxWidth = 0;
	int64 v = inMax;
	while (v > 0)
	{
		v >>= 1;
		++maxWidth;
	}
	
	int32 width = maxWidth;
	int64 last = -1;
	
	uint32 bn[4];
	uint32 dv[4];
	uint32 bc = 0;
	typename std::vector<T>::iterator a = inArray.begin();
	typename std::vector<T>::iterator e = inArray.end();
	
	while (a != e or bc > 0)
	{
		while (bc < 4 and a != e)
		{
			Shift(a, last, dv[bc], bn[bc]);
			++bc;
		}
		
		uint32 s = Select(bn, bc, width, maxWidth, kSelectors);

		if (s == 0)
			width = maxWidth;
		else
			width += kSelectors[s].databits;

		uint32 n = kSelectors[s].span;
		
		WriteBinary(inBits, s, 4);

		if (width > 0)
		{
			for (uint32 i = 0; i < n; ++i)
				WriteBinary(inBits, dv[i], width);
		}
		
		bc -= n;

		if (bc > 0)
		{
			for (uint32 i = 0; i < (4 - n); ++i)
			{
				bn[i] = bn[i + n];
				dv[i] = dv[i + n];
			}
		}
	}
}

template<typename T>
void CompressSimpleArrayGolomb(COBitStream& inBits, std::vector<T>& inArray, int64 inMax)
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

	static void			CompressArray(COBitStream& inBits,
							std::vector<T>& inArray, int64 inMax, uint32 inKind);
};

template<typename T>
void ValuePairTraitsPOD<T>::CompressArray(COBitStream& inBits,
	std::vector<T>& inArray, int64 inMax, uint32 inKind)
{
	switch (inKind)
	{
		case kAC_GolombCode:
			CompressSimpleArrayGolomb(inBits, inArray, inMax);
			break;
		
		case kAC_SelectorCodeV2:
			CompressSimpleArraySelector(inBits, inArray, inMax);
			break;
		
		default:
			THROW(("Unsupported array compression algorithm: %4.4s", &inKind));
	}
}

template<typename T>
struct ValuePairTraitsPair
{
	typedef				std::vector<T>					vector_type;
	typedef typename	vector_type::iterator			iterator;
	typedef typename	vector_type::const_iterator		const_iterator;

	typedef typename	T::first_type					value_type;
	typedef typename	T::second_type					rank_type;

	static void			CompressArray(COBitStream& inBits, std::vector<T>& inArray, int64 inMax,
							uint32 inKind);
};

template<typename T>
void ValuePairTraitsPair<T>::CompressArray(COBitStream& inBits, std::vector<T>& inArray,
	int64 inMax, uint32 inKind)
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
				
			switch (inKind)
			{
				case kAC_GolombCode:
					CompressSimpleArrayGolomb(inBits, values, inMax);
					break;
				
				case kAC_SelectorCodeV2:
					CompressSimpleArraySelector(inBits, values, inMax);
					break;
				
				default:
					THROW(("Unsupported array compression algorithm: %4.4s", &inKind));
			}
		
			values.clear();
			lastWeight = w;
		}
	}
}

// an iterator type for compressed value/rank pairs

template<typename T, uint32 K>
class IteratorBase
{
  public:
					IteratorBase(CIBitStream& inData, int64 inMax)
						: fBits(&inData)
						, fRead(0)
						, fValue(-1)
						, fMax(inMax)
					{
						int64 v = fMax;
						fMaxWidth = 0;
						while (v > 0)
						{
							v >>= 1;
							++fMaxWidth;
						}

						Reset();
					}

	virtual			~IteratorBase() {}

	virtual bool	Next()
					{
						bool done = false;
					
						if (fRead < fCount)
						{
							if (fSpan == 0)
							{
								uint32 selector = ReadBinary<uint32>(*fBits, 4);
								fSpan = kSelectors[selector].span;
								
								if (selector == 0)
									fWidth = fMaxWidth;
								else
									fWidth += kSelectors[selector].databits;
							}
					
							if (fWidth > 0)
								fValue += ReadBinary<int64>(*fBits, fWidth);
							fValue += 1;
					
							--fSpan;
							++fRead;
							
							done = true;
						}
					
						return done;
					}
	
	T				Value() const					{ return static_cast<T>(fValue); }
	virtual uint32	Weight() const					{ return 1; }
	
	virtual uint32	Count() const					{ return fCount; }
	virtual uint32	Read() const					{ return fRead; }

  protected:
					IteratorBase(CIBitStream& inData, int64 inMax, bool)
						: fBits(&inData)
						, fRead(0)
						, fValue(-1)
						, fMax(inMax)
					{
						int64 v = fMax;
						fMaxWidth = 0;
						while (v > 0)
						{
							v >>= 1;
							++fMaxWidth;
						}
					}

					IteratorBase(const IteratorBase& inOther);
	IteratorBase&		operator=(const IteratorBase& inOther);

	void			Reset()
					{
						fValue = -1;
						fCount = ReadGamma(*fBits);
						fRead = 0;
						fSpan = 0;
						fWidth = fMaxWidth;			// backwards compatible
					}

	CIBitStream*	fBits;
	uint32			fCount;
	uint32			fRead;
	int32			fWidth;
	uint32			fMaxWidth;
	uint32			fSpan;

	int64			fValue;
	int64			fMax;
};

template<typename T>
class IteratorBase<T, kAC_GolombCode>
{
  public:
					IteratorBase(CIBitStream& inData, int64 inMax)
						: fBits(&inData)
						, fCount(0)
						, fRead(0)
						, fValue(-1)
						, fMax(inMax)
					{
						Reset();
					}

	virtual			~IteratorBase() {}

	virtual bool	Next()
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
	
	T				Value() const					{ return static_cast<T>(fValue); }
	virtual uint32	Weight() const					{ return 1; }
	
	virtual uint32	Count() const					{ return fCount; }
	virtual uint32	Read() const					{ return fRead; }

  protected:
					IteratorBase(CIBitStream& inData, int64 inMax, bool)
						: fBits(&inData)
						, fCount(0)
						, fRead(0)
						, fValue(-1)
						, fMax(inMax)
					{
					}

					IteratorBase(const IteratorBase& inOther);
	IteratorBase&		operator=(const IteratorBase& inOther);

	void			Reset()
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

	CIBitStream*	fBits;
	uint32			fCount;
	uint32			fRead;
	int32			b, n, g;

	int64			fValue;
	int64			fMax;
};

template<typename T, uint32 K>
class VRIterator : public IteratorBase<typename ValuePairTraitsPair<T>::value_type, K>
{
	typedef IteratorBase<typename ValuePairTraitsPair<T>::value_type, K>	base_type;

	typedef				ValuePairTraitsPair<T>		traits;
	typedef typename	traits::value_type			value_type;
	typedef typename	traits::rank_type			rank_type;
	
  public:
					VRIterator(CIBitStream& inData, int64 inMax)
						: base_type(inData, inMax, false)
					{
						fTotalCount = ReadGamma(inData);
						fWeight = ReadGamma(inData);
						fTotalRead = 0;
						this->Reset();
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

template<typename T, uint32 K>
void VRIterator<T, K>::Restart()
{
	fWeight -= ReadGamma(*this->fBits);
	
//	assert(fWeight <= kMaxWeight);
	assert(fWeight > 0);

	if (fWeight > 0)
		this->Reset();
	else
		this->fCount = 0;

	assert(this->fCount <= fTotalCount - fTotalRead);
}

template<typename T, uint32 K>
bool VRIterator<T, K>::Next()
{
	bool done = false;
	
	while (not done and fWeight > 0 and fTotalRead < fTotalCount)
	{
		if (base_type::Next())
		{
			++fTotalRead;
			done = true;
		}
		else
			Restart();
	}
	
	return done;
}

template<typename T, uint32 K, bool>
struct ValuePairTraitsTypeFactoryBase
{
	typedef	ValuePairTraitsPOD<T>	type;
	typedef IteratorBase<T,K>		iterator;
};

template<typename T, uint32 K>
struct ValuePairTraitsTypeFactoryBase<T, K, false>
{
	typedef	ValuePairTraitsPair<T>	type;
	typedef VRIterator<T,K>			iterator;
};

template<typename T, uint32 K>
struct ValuePairTraitsTypeFactory : public ValuePairTraitsTypeFactoryBase<T, K, boost::is_integral<T>::value>
{
};

}

template<typename T>
void CompressArray(COBitStream& inBits, T& inArray, int64 inMax, uint32 inKind)
{
	using namespace		CValuePairCompression;

	typedef typename	T::value_type										vector_value_type;
	
	switch (inKind)
	{
		case kAC_GolombCode:
		{
			typedef typename	ValuePairTraitsTypeFactory<vector_value_type,kAC_GolombCode>::type	traits;
			traits::CompressArray(inBits, inArray, inMax, inKind);
			break;
		}

		case kAC_SelectorCodeV2:
		{
			typedef typename	ValuePairTraitsTypeFactory<vector_value_type,kAC_SelectorCodeV2>::type traits;
			traits::CompressArray(inBits, inArray, inMax, inKind);
			break;
		}
		
		default:
			THROW(("Unsupported array compression"));
	}
}

#endif // CVALUEPAIRARRAY_H
