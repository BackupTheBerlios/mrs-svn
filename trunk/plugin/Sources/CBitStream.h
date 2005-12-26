/*	$Id: CBitStream.h,v 1.32 2005/08/22 12:38:06 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Sunday December 08 2002 11:36:45
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
 
#ifndef CBITSTREAM_H
#define CBITSTREAM_H

#include "HStlVector.h"
#include "HStdCMath.h"

// bit stream utility classes and routines

class HStreamBase;

class obit_stream
{
  public:
					obit_stream();
					obit_stream(HStreamBase& inFile);
					~obit_stream();
	
	obit_stream&	operator<<(int inBit);
	
	void			sync();
	
	const char*		peek() const;
	uint32			size() const;
	
	uint32			bit_size() const;

  private:
					obit_stream(const obit_stream&);
	obit_stream&	operator=(const obit_stream&);

	void			overflow();

	char*			data;
	uint32			byte_offset;
	uint32			physical_size;
	int8			bit_offset;
	HStreamBase*	file;
};

class ibit_stream
{
  public:
					ibit_stream(HStreamBase& inData, int64 inOffset);
					ibit_stream(HStreamBase& inData, int64 inOffset, uint32 inSize);
					ibit_stream(const char* inData, uint32 inSize);
					ibit_stream(const ibit_stream& inOther);
					~ibit_stream();

	ibit_stream&	operator=(const ibit_stream& inOther);

	bool			operator==(const ibit_stream& inOther) const;
	bool			operator!=(const ibit_stream& inOther) const;
	
	ibit_stream&	operator>>(int& outBit);
	int				next_bit();
	
//	void			skip(uint32 inBits);
	
	bool			eof() const;
	void			underflow();
	
	uint32			bytes_read() const;

  private:

	struct ibit_stream_imp*	impl;

	int8			byte;
	int8			bit_offset;
};

uint32 ReadGamma(ibit_stream& inBits);
void WriteGamma(obit_stream& inBits, uint32 inValue);

uint32 ReadUnary(ibit_stream& inBits);
void WriteUnary(obit_stream& inBits, uint32 inValue);

template<class T>
T ReadBinary(ibit_stream& inBits, int inBitCount)
{
	assert(inBitCount < sizeof(T) * 8);
	T result = 0;

	for (int32 i = inBitCount - 1; i >= 0; --i)
	{
		if (inBits.next_bit())
			result |= 1 << i;
	}
	
	return result;
}

template<class T>
void WriteBinary(obit_stream& inBits, T inValue, int inBitCount)
{
	assert(inBitCount < sizeof(T) * 8);
	for (int32 i = inBitCount - 1; i >= 0; --i)
		inBits << ((inValue & (1 << i)) != 0);
}

template<int bit_size>
class CBinaryOutStream
{
  public:
						CBinaryOutStream(obit_stream& inStream)
							: fBits(inStream)
						{
						}
	
	CBinaryOutStream&	operator<<(uint32 inValue)
	{
		for (int32 i = bit_size - 1; i >= 0; --i)
			fBits << ((inValue & (1 << i)) != 0);
		return *this;
	}

  private:
	obit_stream&		fBits;
};

template<int bit_size>
class CBinaryInStream
{
  public:
						CBinaryInStream(ibit_stream& inStream)
							: fBits(inStream)
						{
						}
	
	template<typename T>
	CBinaryInStream&	operator>>(T& outValue)
	{
		outValue = 0;
		for (int32 i = bit_size - 1; i >= 0; --i)
		{
			if (fBits.next_bit())
				outValue |= 1 << i;
		}
		return *this;
	}

  private:
	ibit_stream&		fBits;
};


template<class T>
void CompressArray(obit_stream& inBits, const T& inArray, int64 inMax);

template<class T>
void DecompressArray(ibit_stream& inBits, T& outArray, int64 inMax);

// inline implementations

inline
obit_stream& obit_stream::operator<<(int inBit)
{
	if (data == NULL)
		overflow();
	
	if (inBit)
		data[byte_offset] |= static_cast<char>(1 << bit_offset);

	if (--bit_offset < 0)
	{
		++byte_offset;
		bit_offset = 7;

		if (byte_offset >= physical_size)
			overflow();

		data[byte_offset] = 0;
	}
	return *this;
}

inline
ibit_stream& ibit_stream::operator>>(int& outBit)
{
	outBit = (byte & (1 << bit_offset)) != 0;
	--bit_offset;

	if (bit_offset < 0)
		underflow();

	return *this;
}

inline
int ibit_stream::next_bit()
{
	int result = (byte & (1 << bit_offset)) != 0;
	--bit_offset;

	if (bit_offset < 0)
		underflow();

	return result;
}

// compressarray implementation:

inline
int32 CalculateB(int64 inMax, uint32 inCnt)
{
	using namespace std;

		// first value is 0 so probability of 1 is cnt / (max + 1) value
	double p = double(inCnt) / (inMax + 1);

	assert(p <= 1.0);

	int32 b;
	if (p == 1.0)	// since we have a slightly modified Golomb code calculation...
		b = 1;
	else
		b = static_cast<int32>(ceil(log(2 - p) / -log(1 - p)));

	if (b < 1)
		b = 1;
	
	return b;
}

template<class T>
void CompressArray(obit_stream& inBits, const T& inArray, int64 inMax)
{
	typedef typename T::const_iterator	iterator;
	
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
	
	int64 l = -1;	// we store delta's and our arrays can start at zero...
	
	for (iterator i = inArray.begin(); i != inArray.end(); ++i)
	{
		int32 d = static_cast<int32>(*i - l);
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
		
		l = *i;
	}
}

void CompressArray(obit_stream& inDstBits, ibit_stream& inSrcBits,
	uint32 inCount, int64 inMax);

template <class T>
void DecompressArray(ibit_stream& inBits, T& outArray, int64 inMax)
{
	uint32 cnt = ReadGamma(inBits);
	int32 b = CalculateB(inMax, cnt);

	int32 n = 0, g = 1;
	while (g < b)
	{
		++n;
		g <<= 1;
	}
	g -= b;

	int64 l = -1;
	
	while (cnt-- > 0)
	{
		int32 q = 0;
		int32 r = 0;
		
		if (inBits.next_bit())
		{
			q = 1;
			while (inBits.next_bit())
				++q;
		}
		
		if (b > 1)
		{
			for (int e = 0; e < n - 1; ++e)
				r = (r << 1) | inBits.next_bit();
			
			if (r >= g)
			{
				r = (r << 1) | inBits.next_bit();
				r -= g;
			}
		}
		
		int32 d = r + q * b + 1;
		
		l += d;
		outArray.push_back(l);
	}
}

#endif // CBITSTREAM_H
