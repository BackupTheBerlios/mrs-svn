/*	$Id$
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

#include <cstdlib>
#include <cstring>
#include <cmath>
#include <limits>
#include <iostream>

#include "HError.h"

#include "CBitStream.h"
#include "HStream.h"

using namespace std;

const uint32
	kBitBufferSize = 64,
	kBitBufferExtend = 1024;

COBitStream::COBitStream()
	: data(new char[kBitBufferSize])
	, byte_offset(0)
	, physical_size(kBitBufferSize)
	, bit_offset(7)
	, file(nil)
{
	data[byte_offset] = 0;
}

COBitStream::COBitStream(HStreamBase& inFile)
	: data(new char[kBitBufferSize])
	, byte_offset(0)
	, physical_size(kBitBufferSize)
	, bit_offset(7)
	, file(&inFile)
{
	data[byte_offset] = 0;
}

COBitStream::~COBitStream()
{
	sync();
	delete[] data;
}

void COBitStream::overflow()
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

void COBitStream::sync()
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

const char* COBitStream::peek() const
{
	assert(bit_offset == 7);
	assert(file == nil);
	return data;
}

uint32 COBitStream::size() const
{
//	assert(bit_offset == 7);
	return byte_offset;
}

uint32 COBitStream::bit_size() const
{
//	assert(bit_offset == 7);
	return (byte_offset + 1) * 8 - bit_offset - 1;
}

// ibit

struct CIBitStreamConstImp : public CIBitStream::CIBitStreamImp
{
					CIBitStreamConstImp(const char* inData, int32 inSize);
	
	virtual void	get(int8& outByte)
					{
						outByte = data[offset++];
						size -= 8;
						++byte_offset;
					}

	virtual CIBitStreamImp*
					clone() const
					{
						CIBitStreamConstImp* result = new CIBitStreamConstImp(data, 0);
						result->offset = offset;
						result->size = size;
						return result;
					}

	const char*		data;
	uint32			offset;
};

CIBitStreamConstImp::CIBitStreamConstImp(const char* inData, int32 inSize)
	: CIBitStreamImp(inSize)
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

struct CIBitStreamFileImp : public CIBitStream::CIBitStreamImp
{
					CIBitStreamFileImp(HStreamBase& inData,
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

	virtual CIBitStreamImp*
					clone() const
					{
						CIBitStreamFileImp* result = new CIBitStreamFileImp(data, offset, 0);
						result->size = size;
						return result;
					}
	
	void			read();
	
	HStreamBase&	data;
	int64			offset;
	char			buffer[kBitBufferExtend];
	uint32			buffer_size;
	char*			buffer_ptr;
};

CIBitStreamFileImp::CIBitStreamFileImp(HStreamBase& inData,
			int64 inOffset, int32 inSize)
	: CIBitStreamImp(inSize)
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

void CIBitStreamFileImp::read()
{
	if (buffer_size == 0)				// avoid reading excess bytes in the first call
		buffer_size = kBitBufferSize;
	else
		buffer_size = kBitBufferExtend;

	if (buffer_size > size)
		buffer_size = size;

	int32 r = data.PRead(buffer, buffer_size, offset);
	if (r < 0)
		THROW(("IO Error in reading bits"));

	buffer_size = r;
	buffer_ptr = buffer;
}

CIBitStream::CIBitStream(HStreamBase& inData, int64 inOffset)
	: bit_offset(7)
{
	HMemoryStream* ms = dynamic_cast<HMemoryStream*>(&inData);
	if (ms != nil)
		impl = new CIBitStreamConstImp(
			reinterpret_cast<const char*>(ms->Buffer()) + inOffset, 0);
	else
		impl = new CIBitStreamFileImp(inData, inOffset, numeric_limits<int32>::max());
	
	impl->get(byte);
}

CIBitStream::CIBitStream(HStreamBase& inData, int64 inOffset, uint32 inSize)
	: bit_offset(7)
{
	HMemoryStream* ms = dynamic_cast<HMemoryStream*>(&inData);
	if (ms != nil)
		impl = new CIBitStreamConstImp(
			reinterpret_cast<const char*>(ms->Buffer()) + inOffset, inSize);
	else
		impl = new CIBitStreamFileImp(inData, inOffset, static_cast<int32>(inSize));
	
	impl->get(byte);
}

CIBitStream::CIBitStream(const char* inData, uint32 inSize)
	: impl(new CIBitStreamConstImp(inData, static_cast<int32>(inSize)))
	, bit_offset(7)
{
	impl->get(byte);
}

CIBitStream::CIBitStream(const CIBitStream& inOther)
	: impl(inOther.impl->clone())
	, byte(inOther.byte)
	, bit_offset(inOther.bit_offset)
{
}

CIBitStream::~CIBitStream()
{
	delete impl;
}

// routines

uint32 ReadUnary(CIBitStream& ioBits)
{
	uint32 v = 1;
	
	while (ioBits.next_bit())
		++v;
	
	return v;
}

void WriteUnary(COBitStream& inBits, uint32 inValue)
{
	assert(inValue > 0);
	
	while (inValue > 1)
	{
		--inValue;
		inBits << 1;
	}

	inBits << 0;
}

uint32 ReadGamma(CIBitStream& ioBits)
{
	uint32 v1 = 1;
	int32 e = 0;
	
	while (ioBits.next_bit() and v1 != 0)
	{
		v1 <<= 1;
		++e;
	}
	
	if (v1 == 0)
		THROW(("incorrect gamma encoding detected"));
	
	int32 v2 = 0;
	while (e-- > 0)
		v2 = (v2 << 1) | ioBits.next_bit();
	
	return v1 + v2;
}

void WriteGamma(COBitStream& inBits, uint32 inValue)
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
