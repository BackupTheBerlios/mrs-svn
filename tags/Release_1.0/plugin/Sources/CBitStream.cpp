/*	$Id: CBitStream.cpp,v 1.33 2005/09/06 14:32:27 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Sunday December 08 2002 14:06:24
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
 
#include "MRS.h"

#include "HStdCStdlib.h"
#include "HStdCString.h"
#include "HStdCMath.h"
#include "HStlLimits.h"
#include "HError.h"

#include "CBitStream.h"
#include "HStream.h"

using namespace std;

const uint32
	kBitBufferSize = 64,
	kBitBufferExtend = 1024;

obit_stream::obit_stream()
	: data(new char[kBitBufferSize])
	, byte_offset(0)
	, physical_size(kBitBufferSize)
	, bit_offset(7)
	, file(nil)
{
	data[byte_offset] = 0;
}

obit_stream::obit_stream(HStreamBase& inFile)
	: data(new char[kBitBufferSize])
	, byte_offset(0)
	, physical_size(kBitBufferSize)
	, bit_offset(7)
	, file(&inFile)
{
	data[byte_offset] = 0;
}

obit_stream::~obit_stream()
{
	sync();
	delete[] data;
}

void obit_stream::overflow()
{
	if (file != nil)
	{
		if (data == nil)
		{
			data = new char[kBitBufferSize];
			physical_size = kBitBufferSize;
		}
		else
			file->Write(data, byte_offset);

		byte_offset = 0;
	}
	else
	{
		char* t = new char[physical_size + kBitBufferExtend];
		if (data != NULL)
		{
			memcpy(t, data, physical_size);
			delete[] data;
		}
		data = t;
		physical_size += kBitBufferExtend;
	}
}

void obit_stream::sync()
{
	(*this) << 0;
	
	while (bit_offset != 7)
		(*this) << 1;
	
	if (file != nil)
	{
		file->Write(data, byte_offset);
		byte_offset = 0;
	}
}

const char* obit_stream::peek() const
{
	assert(bit_offset == 7);
	assert(file == nil);
	return data;
}

uint32 obit_stream::size() const
{
//	assert(bit_offset == 7);
	return byte_offset;
}

uint32 obit_stream::bit_size() const
{
//	assert(bit_offset == 7);
	return (byte_offset + 1) * 8 - bit_offset - 1;
}

// ibit

struct ibit_stream_imp
{
					ibit_stream_imp(int32 inSize)
						: size(inSize), byte_offset(0) {}
	
	virtual			~ibit_stream_imp() {}
	
	virtual void	get(int8& outByte) = 0;
	virtual ibit_stream_imp*
					clone() const = 0;

	int32			size;
	uint32			byte_offset;
};

struct const_ibit_stream_imp : public ibit_stream_imp
{
					const_ibit_stream_imp(const char* inData, int32 inSize);
	
	virtual void	get(int8& outByte)
					{
						outByte = data[offset++];
						size -= 8;
						++byte_offset;
					}

	virtual ibit_stream_imp*
					clone() const
					{
						const_ibit_stream_imp* result = new const_ibit_stream_imp(data, 0);
						result->offset = offset;
						result->size = size;
						return result;
					}

	const char*		data;
	uint32			offset;
};

const_ibit_stream_imp::const_ibit_stream_imp(const char* inData, int32 inSize)
	: ibit_stream_imp(inSize)
	, data(inData)
	, offset(0)
{
	if (size > 0)
	{
		int8 byte = data[size - 1];
		
		size *= 8;
		int32 bit_offset = 0;
	
		while (bit_offset < 7 && byte & (1 << bit_offset) && size > 0)
			++bit_offset, --size;
	
		assert(size > 0);
		--size;
	}
	else
		size = numeric_limits<int32>::max();
}

struct file_ibit_stream_imp : public ibit_stream_imp
{
					file_ibit_stream_imp(HStreamBase& inData,
						int64 inOffset, int32 inSize);
	
	virtual void	get(int8& outByte)
					{
						if (buffer_ptr >= buffer + buffer_size)
							read();
						
						assert(buffer_ptr >= buffer and buffer_ptr < buffer + buffer_size);
						
						outByte = *buffer_ptr++;
						
						size -= 8;
						offset += 1;
						++byte_offset;
					}

	virtual ibit_stream_imp*
					clone() const
					{
						file_ibit_stream_imp* result = new file_ibit_stream_imp(data, offset, 0);
						result->size = size;
						return result;
					}
	
	void			read();
	
	HStreamBase&	data;
	int64			offset;
	char			buffer[kBitBufferSize];
	uint32			buffer_size;
	char*			buffer_ptr;
};

file_ibit_stream_imp::file_ibit_stream_imp(HStreamBase& inData,
			int64 inOffset, int32 inSize)
	: ibit_stream_imp(inSize)
	, data(inData)
	, offset(inOffset)
	, buffer_size(0)
	, buffer_ptr(buffer)
{
	if (inSize > 0 && inSize < numeric_limits<int32>::max())
	{
		int8 byte;
		
		data.PRead(&byte, 1, inOffset + inSize - 1);
		
		size *= 8;
		int32 bit_offset = 0;
		while (bit_offset < 7 && byte & (1 << bit_offset) && size > 0)
			++bit_offset, --size;

		assert(size > 0);
		--size;
	}
}

void file_ibit_stream_imp::read()
{
	buffer_size = kBitBufferSize;
	if (buffer_size > size)
		buffer_size = size;
	
	int32 r = data.PRead(buffer, buffer_size, offset);
	if (r < 0)
		THROW(("IO Error in reading bits"));
	buffer_size = r;
	buffer_ptr = buffer;
}

ibit_stream::ibit_stream(HStreamBase& inData, int64 inOffset)
	: impl(new file_ibit_stream_imp(inData, inOffset, numeric_limits<int32>::max()))
	, bit_offset(7)
{
	impl->get(byte);
}

ibit_stream::ibit_stream(HStreamBase& inData, int64 inOffset, uint32 inSize)
	: impl(new file_ibit_stream_imp(inData, inOffset, static_cast<int32>(inSize)))
	, bit_offset(7)
{
	impl->get(byte);
}

ibit_stream::ibit_stream(const char* inData, uint32 inSize)
	: impl(new const_ibit_stream_imp(inData, static_cast<int32>(inSize)))
	, bit_offset(7)
{
	impl->get(byte);
}

ibit_stream::ibit_stream(const ibit_stream& inOther)
	: impl(inOther.impl->clone())
	, byte(inOther.byte)
	, bit_offset(inOther.bit_offset)
{
}

ibit_stream::~ibit_stream()
{
	delete impl;
}

bool ibit_stream::eof() const
{
	return (7 - bit_offset) >= (8 + impl->size);
}

void ibit_stream::underflow()
{
	if (not eof())
	{
		impl->get(byte);
		bit_offset = 7;
	}
}

uint32 ibit_stream::bytes_read() const
{
	return impl->byte_offset;
}

// routines

uint32 ReadUnary(ibit_stream& ioBits)
{
	uint32 v = 1;
	
	while (ioBits.next_bit())
		++v;
	
	return v;
}

void WriteUnary(obit_stream& inBits, uint32 inValue)
{
	assert(inValue > 0);
	
	while (inValue > 1)
	{
		--inValue;
		inBits << 1;
	}

	inBits << 0;
}

uint32 ReadGamma(ibit_stream& ioBits)
{
	uint32 v1 = 1;
	int32 e = 0;
	
	while (ioBits.next_bit())
	{
		v1 <<= 1;
		++e;
	}
	
	int32 v2 = 0;
	while (e-- > 0)
		v2 = (v2 << 1) | ioBits.next_bit();
	
	return v1 + v2;
}

void WriteGamma(obit_stream& inBits, uint32 inValue)
{
	assert(inValue > 0);
	
	uint32 v = inValue;
	int32 e = 0;
	
	while (v > 1)
	{
		v >>= 1;
		++e;
		inBits << 1;
	}
	inBits << 0;
	
	v = inValue - (1 << e);
	
	while (e-- > 0)
		inBits << ((v & (1 << e)) != 0);
}

void CompressArray(obit_stream& inDstBits, ibit_stream& inSrcBits,
	uint32 inCount, int64 inMax)
{
	int32 b = CalculateB(inMax, inCount);
	
	int32 n = 0, g = 1;
	while (g < b)
	{
		++n;
		g <<= 1;
	}
	g -= b;

	WriteGamma(inDstBits, inCount);
	
	for (uint32 i = 0; i < inCount; ++i)
	{
		int32 d = ReadGamma(inSrcBits);
		assert(d > 0);
		
		int32 q = (d - 1) / b;
		int32 r = d - q * b - 1;
		
		while (q-- > 0)
			inDstBits << 1;
		inDstBits << 0;
		
		if (b > 1)
		{
			if (r < g)
			{
				for (int t = 1 << (n - 2); t != 0; t >>= 1)
					inDstBits << ((r & t) != 0);
			}		
			else
			{
				r += g;
				for (int t = 1 << (n - 1); t != 0; t >>= 1)
					inDstBits << ((r & t) != 0);
			}
		}
	}
}


