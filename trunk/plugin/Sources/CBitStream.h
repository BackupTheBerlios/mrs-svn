/*	$Id: CBitStream.h 331 2007-02-12 07:44:10Z hekkel $
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

#include <vector>
#include <cmath>

// bit stream utility classes and routines

class HStreamBase;

// --------------------------------------------------------------------
//
// COBitStream, class used to stream bits to a 'file'
//

class COBitStream
{
  public:
					COBitStream();
					COBitStream(HStreamBase& inFile);
					~COBitStream();
	
	COBitStream&	operator<<(int inBit);
	
	void			sync();
	
	const char*		peek() const;
	uint32			size() const;
	
	uint32			bit_size() const;

  private:
					COBitStream(const COBitStream&);
	COBitStream&	operator=(const COBitStream&);

	void			overflow();

	char*			data;
	uint32			byte_offset;
	uint32			physical_size;
	int8			bit_offset;
	HStreamBase*	file;
};

inline
COBitStream& COBitStream::operator<<(int inBit)
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

// --------------------------------------------------------------------
//
// CIBitStream, class used to read bits from a data stream
//

class CIBitStream
{
  public:

	struct CIBitStreamImp
	{
						CIBitStreamImp(int32 inSize)
							: size(inSize), byte_offset(0) {}
		
		virtual			~CIBitStreamImp() {}
		
		virtual void	get(int8& outByte) = 0;
		virtual CIBitStreamImp*
						clone() const = 0;
	
		int32			size;
		uint32			byte_offset;
	};

					CIBitStream(HStreamBase& inData, int64 inOffset);
					CIBitStream(HStreamBase& inData, int64 inOffset, uint32 inSize);
					CIBitStream(const char* inData, uint32 inSize);
					CIBitStream(const CIBitStream& inOther);
					~CIBitStream();

	CIBitStream&	operator=(const CIBitStream& inOther);

	bool			operator==(const CIBitStream& inOther) const;
	bool			operator!=(const CIBitStream& inOther) const;
	
	CIBitStream&	operator>>(int& outBit);
	int				next_bit();
	
//	void			skip(uint32 inBits);
	
	bool			eof() const;
	void			underflow();
	
	uint32			bytes_read() const;

  private:

	struct CIBitStreamImp*	impl;

	int8			byte;
	int8			bit_offset;
};

// make sure the inlines are visible before being used!

inline
bool CIBitStream::eof() const
{
	return (7 - bit_offset) >= (8 + impl->size);
}

inline
void CIBitStream::underflow()
{
	if (not eof())
	{
		impl->get(byte);
		bit_offset = 7;
	}
}

inline
uint32 CIBitStream::bytes_read() const
{
	return impl->byte_offset;
}

inline
int CIBitStream::next_bit()
{
	int result = (byte & (1 << bit_offset)) != 0;
	--bit_offset;

	if (bit_offset < 0)
		underflow();

	return result;
}

inline
CIBitStream& CIBitStream::operator>>(int& outBit)
{
	outBit = next_bit();

	return *this;
}

// --------------------------------------------------------------------
//
// Several global functions used to read from/write to the bit streams
//

uint32 ReadGamma(CIBitStream& inBits);
void WriteGamma(COBitStream& inBits, uint32 inValue);

uint32 ReadUnary(CIBitStream& inBits);
void WriteUnary(COBitStream& inBits, uint32 inValue);

template<class T>
inline
T ReadBinary(CIBitStream& inBits, int inBitCount)
{
	assert(inBitCount < sizeof(T) * 8);
	assert(inBitCount > 0);
	T result = 0;

	for (int32 i = inBitCount - 1; i >= 0; --i)
	{
		if (inBits.next_bit())
			result |= 1 << i;
	}
	
	return result;
}

template<class T>
inline
void WriteBinary(COBitStream& inBits, T inValue, int inBitCount)
{
	assert(inBitCount <= sizeof(T) * 8);
	assert(inBitCount > 0);
	for (int32 i = inBitCount - 1; i >= 0; --i)
		inBits << ((inValue & (1 << i)) != 0);
}

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

#endif // CBITSTREAM_H
