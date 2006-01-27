/*	$Id: CDecompress.cpp,v 1.54 2005/08/22 12:38:06 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Saturday December 07 2002 21:01:35
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

#include "HError.h"
#include "HUtils.h"
#include "HStream.h"
#include "HFileCache.h"

#include "CDecompress.h"
#include "CCompress.h"
#include "CIndexPage.h"
#include "CBitStream.h"
#include "CUtils.h"
#include "CArray.h"

#include "zlib.h"
#include "bzlib.h"

using namespace std;

struct CDecompressorImp
{
					CDecompressorImp(HStreamBase& inFile);
	virtual			~CDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize) = 0;
	
	static CDecompressorImp* Create(HStreamBase& inFile, uint32 inKind);

	HStreamBase&	fFile;
	uint32			fDictLength;
};

CDecompressorImp::CDecompressorImp(HStreamBase& inFile)
	: fFile(inFile)
	, fDictLength(0)
{
}

CDecompressorImp::~CDecompressorImp()
{
}

// compression using zlib

struct CZLibDecompressorImp : public CDecompressorImp
{
					CZLibDecompressorImp(HStreamBase& inFile);
	virtual			~CZLibDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);

	z_stream_s		z_stream;
	unsigned char*	dictionary;
};

CZLibDecompressorImp::CZLibDecompressorImp(HStreamBase& inFile)
	: CDecompressorImp(inFile)
	, dictionary(NULL)
{
	HSwapStream<net_swapper> data(fFile);
	
	// read dictionary
	uint16 n;
	data >> n;

	fDictLength = n;
	dictionary = new unsigned char[fDictLength];

	fFile.Read(dictionary, fDictLength);
	
	memset(&z_stream, 0, sizeof(z_stream));

	int err = inflateInit(&z_stream);
	if (err != Z_OK)
		THROW(("Decompression error: %s", z_stream.msg));
}

CZLibDecompressorImp::~CZLibDecompressorImp()
{
	delete dictionary;
	inflateEnd(&z_stream);
}
	
string CZLibDecompressorImp::GetDocument(int64 inOffset, uint32 inLength)
{
	string result;

	const long kBufferSize = 0x10000;
	
	HAutoBuf<unsigned char> src_buf(new unsigned char[kBufferSize]);
	HAutoBuf<unsigned char> dst_buf(new unsigned char[kBufferSize]);

	z_stream.next_in = src_buf.get();
	z_stream.avail_in = 0;
	z_stream.total_in = 0;
	
	z_stream.next_out = dst_buf.get();
	z_stream.avail_out = kBufferSize;
	z_stream.total_out = 0;

	bool done = false;
	int err = inflateReset(&z_stream);

	while (not done && err >= Z_OK)
	{
			// shift remaining unread bytes to the beginning of our buffer
		if (z_stream.next_in > src_buf.get() && z_stream.avail_in > 0)
			memmove(src_buf.get(), z_stream.next_in, z_stream.avail_in);

		z_stream.next_in = src_buf.get();

			// flush parameter, if we still have data set it to 0
		int flush = 0;
		if (inLength > 0)
		{
			uint32 size = kBufferSize - z_stream.avail_in;
			if (size > inLength)
				size = static_cast<uint32>(inLength);
			inLength -= size;

			inOffset += fFile.PRead(src_buf.get() + z_stream.avail_in, size, inOffset);
			z_stream.avail_in += size;
		}
		else
			flush = Z_SYNC_FLUSH;

		err = inflate(&z_stream, flush);
		if (err == Z_NEED_DICT)
			err = inflateSetDictionary(&z_stream, dictionary, fDictLength);

		if (err == Z_STREAM_END)
			done = true;
		
		if (kBufferSize - z_stream.avail_out > 0)
		{
			result.append(reinterpret_cast<char*>(dst_buf.get()),
				kBufferSize - z_stream.avail_out);
			z_stream.avail_out = kBufferSize;
			z_stream.next_out = dst_buf.get();
		}
	}
	
	if (err < Z_OK)
		THROW(("Decompression error: %s (%d)", z_stream.msg, err));
	
	return result;
}

// compression using libbzip2

struct CbzLibDecompressorImp : public CDecompressorImp
{
					CbzLibDecompressorImp(HStreamBase& inFile);
	virtual			~CbzLibDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);
};

CbzLibDecompressorImp::CbzLibDecompressorImp(HStreamBase& inFile)
	: CDecompressorImp(inFile)
{
}

CbzLibDecompressorImp::~CbzLibDecompressorImp()
{
}
	
string CbzLibDecompressorImp::GetDocument(int64 inOffset, uint32 inLength)
{
	string result;

	bz_stream stream = { 0 };
	int err = BZ2_bzDecompressInit(&stream, 0, 0);
	if (err != BZ_OK)
		THROW(("Decompression error: %d", err));

	const long kBufferSize = 0x10000;
	
	HAutoBuf<char> src_buf(new char[kBufferSize]);
	HAutoBuf<char> dst_buf(new char[kBufferSize]);

	stream.next_in = src_buf.get();
	stream.avail_in = 0;
	
	stream.next_out = dst_buf.get();
	stream.avail_out = kBufferSize;

	bool done = false;
	while (not done && err >= Z_OK)
	{
			// shift remaining unread bytes to the beginning of our buffer
		if (stream.next_in > src_buf.get() && stream.avail_in > 0)
			memmove(src_buf.get(), stream.next_in, stream.avail_in);

		stream.next_in = src_buf.get();

			// flush parameter, if we still have data set it to 0
		if (inLength > 0)
		{
			uint32 size = kBufferSize - stream.avail_in;
			if (size > inLength)
				size = static_cast<uint32>(inLength);
			inLength -= size;

			inOffset += fFile.PRead(src_buf.get() + stream.avail_in, size, inOffset);
			stream.avail_in += size;
		}

		err = BZ2_bzDecompress(&stream);

		if (kBufferSize - stream.avail_out > 0)
		{
			result.append(dst_buf.get(), kBufferSize - stream.avail_out);
			stream.avail_out = kBufferSize;
			stream.next_out = dst_buf.get();
		}

		if (err == BZ_STREAM_END)
			done = true;
	}
	
	BZ2_bzDecompressEnd(&stream);
	
	if (err < BZ_OK)
		THROW(("Decompression error: %d", err));
	
	return result;
}

// compression using huffword

struct CHuffWordDecompressorImp : public CDecompressorImp
{
					CHuffWordDecompressorImp(HStreamBase& inFile);
	virtual			~CHuffWordDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);

	char*			buffer;
	uint32			buffer_size, buffer_offset, data_offset;
	int32			bit_offset;
	int64			bit_cnt;
	bool			eof;
	
	char*			word_table;
	uint32			word_six[32];
	uint32			word_firstcode[32];
	uint32*			word_symbol_table;
	char*			word_char_table;
	uint32			word_char_six[32];
	uint32			word_char_firstcode[32];

	char*			nonword_table;
	uint32			nonword_six[32];
	uint32			nonword_firstcode[32];
	uint32*			nonword_symbol_table;
	char*			nonword_char_table;
	uint32			nonword_char_six[32];
	uint32			nonword_char_firstcode[32];
};

CHuffWordDecompressorImp::CHuffWordDecompressorImp(HStreamBase& inFile)
	: CDecompressorImp(inFile)
{
	int64 startOffset = inFile.Tell();
	
	HSwapStream<net_swapper> data(fFile);

	// setup decompression tables
	uint32 n, i, s, l;

	data >> n;
	for (i = 0; i < 32; ++i)	data >> word_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> word_six[i];

	word_symbol_table = new uint32[n];
	data >> s;
	word_table = new char[s];
	fFile.Read(word_table, s);
	
	char* p = word_table;
	char* m = p + s;
	
	word_symbol_table[0] = 0;
	for (i = 1; i < n; ++i)
	{
		l = strlen(p) + 1;
		word_symbol_table[i] = word_symbol_table[i - 1] + l;
		p += l;
		if (p > m)
			break;
	}
	
	data >> n;
	for (i = 0; i < 32; ++i)	data >> word_char_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> word_char_six[i];
	word_char_table = new char[n];
	if (n > 0) fFile.Read(word_char_table, n);

//	fFile.Seek(header.nonword_lexicon, eSeekBeg);
	data >> n;
	for (i = 0; i < 32; ++i)	data >> nonword_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> nonword_six[i];
	nonword_symbol_table = new uint32[n];
	data >> s;
	nonword_table = new char[s];
	fFile.Read(nonword_table, s);

	p = nonword_table;
	m = p + s;
	
	nonword_symbol_table[0] = 0;
	for (i = 1; i < n; ++i)
	{
		l = strlen(p) + 1;
		nonword_symbol_table[i] = nonword_symbol_table[i - 1] + l;
		p += l;
		if (p > m)
			break;
	}

	data >> n;
	for (i = 0; i < 32; ++i)	data >> nonword_char_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> nonword_char_six[i];
	nonword_char_table = new char[n];
	if (n > 0) fFile.Read(nonword_char_table, n);
	
	buffer = NULL;
	fDictLength = fFile.Tell() - startOffset;
}

CHuffWordDecompressorImp::~CHuffWordDecompressorImp()
{
	delete[] word_table;
	delete[] word_char_table;
	delete[] nonword_table;
	delete[] nonword_char_table;
}

string CHuffWordDecompressorImp::GetDocument(int64 inOffset, uint32 inSize)
{
	string result;

	CIBitStream bits(fFile, inOffset, inSize);
	
	bool word = true;
	uint32 v;
	uint32 l;
	uint32 ix;
	const char* s;
	
	while (not bits.eof())
	{
		v = bits.next_bit() != 0;
		l = 1;
		
		if (word)
		{
			while (v < word_firstcode[l])
			{
				v = (v << 1) + bits.next_bit();
				++l;
			}
			
			ix = word_symbol_table[word_six[l] + v - word_firstcode[l]];
			s = word_table + ix;
			
			if (strcmp(s, "\x1b"))
			{
				result += s;
			}
			else
			{
				while (not bits.eof())
				{
					v = bits.next_bit() != 0;
					l = 1;
					
					while (v < word_char_firstcode[l])
					{
						v = (v << 1) + bits.next_bit();
						++l;
					}
					
					char c = word_char_table[word_char_six[l] + v - word_char_firstcode[l]];
					
					if (c == 0)
						break;
					
					result += c;
				}
			}
		}
		else
		{
			while (v < nonword_firstcode[l])
			{
				v = (v << 1) + bits.next_bit();
				++l;
			}

			ix = nonword_symbol_table[nonword_six[l] + v - nonword_firstcode[l]];
			s = nonword_table + ix;
			
			if (strcmp(s, "\x1b"))
			{
				result += s;
			}
			else
			{
				while (not bits.eof())
				{
					v = bits.next_bit() != 0;
					l = 1;
					
					while (v < nonword_char_firstcode[l])
					{
						v = (v << 1) + bits.next_bit();
						++l;
					}
					
					char c = nonword_char_table[nonword_char_six[l] + v - nonword_char_firstcode[l]];
					
					if (c == 0)
						break;
					
					result += c;
				}
			}
		}

		word = not word;
	}
	
	return result;
}

CDecompressorImp* CDecompressorImp::Create(HStreamBase& inFile, uint32 inKind)
{
	CDecompressorImp* result = NULL;
	
	switch (inKind)
	{
		case kHuffWordCompressed:
			result = new CHuffWordDecompressorImp(inFile);
			break;
		
		case kZLibCompressed:
			result = new CZLibDecompressorImp(inFile);
			break;
		
		case kbzLibCompressed:
			result = new CbzLibDecompressorImp(inFile);
			break;
		
		default:
			THROW(("Invalid data file"));
	}

	return result;
}

CDecompressor::CDecompressor(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize,
		int64 inTableOffset, int64 inTableSize)
	: fImpl(nil)
	, fDocIndex(NULL)
	, fKind(inKind)
	, fFile(inFile)
	, fDataOffset(inDataOffset)
	, fDataSize(inDataSize)
	, fTableOffset(inTableOffset)
	, fTableSize(inTableSize)
	, fDictLength(0)
{
	inFile.Seek(inDataOffset, SEEK_SET);

	fImpl = CDecompressorImp::Create(inFile, inKind);	

	fDictLength = inFile.Tell() - inDataOffset;

	fDocIndex = new CCArray<int64>(inFile, inTableOffset,
		inTableSize, inDataSize);
}

CDecompressor::~CDecompressor()
{
	delete fImpl;
	delete fDocIndex;
}

string CDecompressor::GetDocument(uint32 inEntry)
{
	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inEntry];
	size = static_cast<uint32>((*fDocIndex)[inEntry + 1] - offset);

	assert(offset + size <= fDataSize - fDictLength);

	return fImpl->GetDocument(
		fDataOffset + fDictLength + offset, size);
}

// -- iterator

CDecompressor::iterator::iterator(CDecompressor& inData)
	: data(inData)
	, doc_nr(0)
{
	doc_offset = (*data.fDocIndex)[doc_nr];
}

bool CDecompressor::iterator::Next(std::string& outDoc)
{
	if (doc_nr >= data.fDocIndex->Size())
		return false;
	
	++doc_nr;
	int64 offset = (*data.fDocIndex)[doc_nr];

	uint32 size = offset - doc_offset;
	
	outDoc = data.fImpl->GetDocument(
		data.fDataOffset + data.fDictLength + doc_offset, size);
	
	doc_offset = offset;
	
	return true;
}

void CDecompressor::CopyData(HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize)
{
	try
	{
		HFileCache::BypassCache(true);

		const int kBufferSize = 1024 * 1024 * 4;	// 4 Mb
		HAutoBuf<char> buf(new char[kBufferSize]);
		
		outKind = fKind;
		outDataOffset = outData.Seek(0, SEEK_END);
		outDataSize = fDataSize;
		
		int64 k = fDataSize;
		int64 o = fDataOffset;
		while (k > 0)
		{
			int64 n = k;
			if (n > kBufferSize)
				n = kBufferSize;
			
			fFile.PRead(buf.get(), static_cast<uint32>(n), o);
			outData.Write(buf.get(), static_cast<uint32>(n));
			
			k -= n;
			o += n;
		}

		outTableOffset = outData.Tell();
		outTableSize = fTableSize;
		
		k = fTableSize;
		o = fTableOffset;
		while (k > 0)
		{
			int64 n = k;
			if (n > kBufferSize)
				n = kBufferSize;
			
			fFile.PRead(buf.get(), static_cast<uint32>(n), o);
			outData.Write(buf.get(), static_cast<uint32>(n));
			
			k -= n;
			o += n;
		}
		
		HFileCache::BypassCache(false);
	}
	catch (...)
	{
		HFileCache::BypassCache(false);
		throw;
	}
}
