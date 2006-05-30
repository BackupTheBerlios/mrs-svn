/*	$Id$
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
#include "HFile.h"

#include "CDecompress.h"
#include "CCompress.h"
#include "CIndex.h"
#include "CBitStream.h"
#include "CUtils.h"
#include "CArray.h"

#include "zlib.h"
#include "bzlib.h"

using namespace std;

struct CDecompressorImp
{
					CDecompressorImp(HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	virtual			~CDecompressorImp();
	
	virtual string	GetDocument(uint32 inDocNr) = 0;

	virtual void	CopyData(HStreamBase& outData, uint32& outKind,
						int64& outDataOffset, int64& outDataSize,
						int64& outTableOffset, int64& outTableSize);

	virtual void	LinkData(const std::string& inDataFileName,
							const std::string& inDataFileUUID,
							HStreamBase& outData, uint32& outKind,
							int64& outDataOffset, int64& outDataSize,
							int64& outTableOffset, int64& outTableSize);
	
	static CDecompressorImp*
					Create(const HUrl& inDb,
						HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);

	virtual void	Init();
	
	HStreamBase*	fFile;
	uint32			fKind;
	int64			fDataOffset, fDataSize;
	int64			fTableOffset, fTableSize;
};

CDecompressorImp::CDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: fFile(&inFile)
	, fKind(inKind)
	, fDataOffset(inDataOffset)
	, fDataSize(inDataSize)
	, fTableOffset(inTableOffset)
	, fTableSize(inTableSize)
{
}

CDecompressorImp::~CDecompressorImp()
{
}

void CDecompressorImp::Init()
{
	fFile->Seek(fDataOffset, SEEK_SET);
}

void CDecompressorImp::LinkData(const std::string& inDataFileName,
	const std::string& inDataFileUUID, HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize, int64& outTableOffset, int64& outTableSize)
{
	HSwapStream<net_swapper> data(outData);
	
	outDataOffset = outData.Seek(0, SEEK_END);
	
	data << inDataFileName << inDataFileUUID
		 << fKind
		 << fDataOffset << fDataSize
		 << fTableOffset << fTableSize;
	
	outDataSize = outData.Tell() - outDataOffset;
	outTableOffset = 0;
	outTableSize = 0;
	
	outKind = kLinkedData;
}

void CDecompressorImp::CopyData(HStreamBase& outData, uint32& outKind,
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
			
			fFile->PRead(buf.get(), static_cast<uint32>(n), o);
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
			
			fFile->PRead(buf.get(), static_cast<uint32>(n), o);
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

struct CBasicDecompressorImp : public CDecompressorImp
{
					CBasicDecompressorImp(HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	virtual			~CBasicDecompressorImp();

	virtual void	Init();

	virtual string	GetDocument(uint32 inDocNr);
	virtual string	GetDocument(int64 inOffset, uint32 inSize) = 0;

//	virtual void	CopyData(HStreamBase& outData, uint32& outKind,
//						int64& outDataOffset, int64& outDataSize,
//						int64& outTableOffset, int64& outTableSize);
//	
//	virtual void	LinkData(const HUrl& inMergedDb,
//						HStreamBase& outData, uint32& outKind,
//						int64& outDataOffset, int64& outDataSize,
//						int64& outTableOffset, int64& outTableSize);

	CCArray<int64>*	fDocIndex;
};

CBasicDecompressorImp::CBasicDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: CDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize)
	, fDocIndex(NULL)
{
}

CBasicDecompressorImp::~CBasicDecompressorImp()
{
	delete fDocIndex;
}

void CBasicDecompressorImp::Init()
{
	CDecompressorImp::Init();
	
	fDocIndex = new CCArray<int64>(*fFile, fTableOffset, fTableSize, fDataSize);
}

string CBasicDecompressorImp::GetDocument(uint32 inDocNr)
{
	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inDocNr];
	size = static_cast<uint32>((*fDocIndex)[inDocNr + 1] - offset);

	assert(offset + size <= fDataSize);

	return GetDocument(fDataOffset + offset, size);
}

// compression using zlib

struct CZLibDecompressorImp : public CBasicDecompressorImp
{
					CZLibDecompressorImp(HStreamBase& inFile,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	virtual			~CZLibDecompressorImp();
	
	virtual void	Init();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);

	z_stream_s		fZStream;
	unsigned char*	fDictionary;
	uint32			fDictLength;
};

CZLibDecompressorImp::CZLibDecompressorImp(HStreamBase& inFile,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: CBasicDecompressorImp(inFile, kZLibCompressed, inDataOffset, inDataSize, inTableOffset, inTableSize)
	, fDictionary(NULL)
{
}

CZLibDecompressorImp::~CZLibDecompressorImp()
{
	delete fDictionary;
	inflateEnd(&fZStream);
}

void CZLibDecompressorImp::Init()
{
	CBasicDecompressorImp::Init();

	HSwapStream<net_swapper> data(*fFile);
	
	// read fDictionary
	uint16 n;
	data >> n;

	fDictLength = n;
	fDictionary = new unsigned char[fDictLength];

	fFile->Read(fDictionary, fDictLength);
	
	memset(&fZStream, 0, sizeof(fZStream));

	int err = inflateInit(&fZStream);
	if (err != Z_OK)
		THROW(("Decompression error: %s", fZStream.msg));
}
	
string CZLibDecompressorImp::GetDocument(int64 inOffset, uint32 inLength)
{
	inOffset += sizeof(uint16) + fDictLength;
	
	string result;

	const long kBufferSize = 0x10000;
	
	HAutoBuf<unsigned char> src_buf(new unsigned char[kBufferSize]);
	HAutoBuf<unsigned char> dst_buf(new unsigned char[kBufferSize]);

	fZStream.next_in = src_buf.get();
	fZStream.avail_in = 0;
	fZStream.total_in = 0;
	
	fZStream.next_out = dst_buf.get();
	fZStream.avail_out = kBufferSize;
	fZStream.total_out = 0;

	bool done = false;
	int err = inflateReset(&fZStream);

	while (not done && err >= Z_OK)
	{
			// shift remaining unread bytes to the beginning of our buffer
		if (fZStream.next_in > src_buf.get() && fZStream.avail_in > 0)
			memmove(src_buf.get(), fZStream.next_in, fZStream.avail_in);

		fZStream.next_in = src_buf.get();

			// flush parameter, if we still have data set it to 0
		int flush = 0;
		if (inLength > 0)
		{
			uint32 size = kBufferSize - fZStream.avail_in;
			if (size > inLength)
				size = static_cast<uint32>(inLength);
			inLength -= size;

			inOffset += fFile->PRead(src_buf.get() + fZStream.avail_in, size, inOffset);
			fZStream.avail_in += size;
		}
		else
			flush = Z_SYNC_FLUSH;

		err = inflate(&fZStream, flush);
		if (err == Z_NEED_DICT)
			err = inflateSetDictionary(&fZStream, fDictionary, fDictLength);

		if (err == Z_STREAM_END)
			done = true;
		
		if (kBufferSize - fZStream.avail_out > 0)
		{
			result.append(reinterpret_cast<char*>(dst_buf.get()),
				kBufferSize - fZStream.avail_out);
			fZStream.avail_out = kBufferSize;
			fZStream.next_out = dst_buf.get();
		}
	}
	
	if (err < Z_OK)
		THROW(("Decompression error: %s (%d)", fZStream.msg, err));
	
	return result;
}

// compression using libbzip2

struct CbzLibDecompressorImp : public CBasicDecompressorImp
{
					CbzLibDecompressorImp(HStreamBase& inFile,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	virtual			~CbzLibDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);
};

CbzLibDecompressorImp::CbzLibDecompressorImp(HStreamBase& inFile,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: CBasicDecompressorImp(inFile, kbzLibCompressed, inDataOffset, inDataSize, inTableOffset, inTableSize)
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

			inOffset += fFile->PRead(src_buf.get() + stream.avail_in, size, inOffset);
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

struct CHuffWordDecompressorImp : public CBasicDecompressorImp
{
					CHuffWordDecompressorImp(HStreamBase& inFile,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	virtual			~CHuffWordDecompressorImp();

	virtual void	Init();
	
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
	
	uint32			offset;
};

CHuffWordDecompressorImp::CHuffWordDecompressorImp(HStreamBase& inFile,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: CBasicDecompressorImp(inFile, kHuffWordCompressed, inDataOffset, inDataSize, inTableOffset, inTableSize)
	, offset(0)
{
}

void CHuffWordDecompressorImp::Init()
{
	int64 startOffset = fFile->Tell();
	
	HSwapStream<net_swapper> data(*fFile);

	// setup decompression tables
	uint32 n, i, s, l;

	data >> n;
	for (i = 0; i < 32; ++i)	data >> word_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> word_six[i];

	word_symbol_table = new uint32[n];
	data >> s;
	word_table = new char[s];
	fFile->Read(word_table, s);
	
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
	if (n > 0) fFile->Read(word_char_table, n);

//	fFile->Seek(header.nonword_lexicon, eSeekBeg);
	data >> n;
	for (i = 0; i < 32; ++i)	data >> nonword_firstcode[i];
	for (i = 0; i < 32; ++i)	data >> nonword_six[i];
	nonword_symbol_table = new uint32[n];
	data >> s;
	nonword_table = new char[s];
	fFile->Read(nonword_table, s);

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
	if (n > 0) fFile->Read(nonword_char_table, n);
	
	buffer = NULL;
	offset = fFile->Tell() - startOffset;
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
	inOffset += offset;
	
	string result;

	CIBitStream bits(*fFile, inOffset, inSize);
	
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

class CLinkedDataImp : public CDecompressorImp
{
  public:
					CLinkedDataImp(const HUrl& inURL, HStreamBase& inFile,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize);
	
	virtual string	GetDocument(uint32 inDocNr);

	virtual void	LinkData(const std::string& inDataFileName,
							const std::string& inDataFileUUID,
							HStreamBase& outData, uint32& outKind,
							int64& outDataOffset, int64& outDataSize,
							int64& outTableOffset, int64& outTableSize);

  private:
	auto_ptr<CDecompressorImp>		fDecompressor;
	auto_ptr<HBufferedFileStream>	fMyFile;
	string							fDataFileName, fDataFileUUID;
};

CLinkedDataImp::CLinkedDataImp(const HUrl& inURL, HStreamBase& inFile,
	int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
	: CDecompressorImp(inFile, 0, 0, 0, 0, 0)
{
	HStreamView view(inFile, inDataOffset, inDataSize);
	HSwapStream<net_swapper> data(view);
	
	data >> fDataFileName >> fDataFileUUID
		 >> fKind
		 >> fDataOffset >> fDataSize
		 >> fTableOffset >> fTableSize;
	
	HUrl dir(inURL.GetParent());
	HUrl url(dir.GetChild(fDataFileName));
	
	if (not HFile::Exists(url))
		THROW(("Linked data file (%s) not found", url.GetURL().c_str()));
	
	CDatabank db(url, false);
	
	if (db.GetUUID() != fDataFileUUID)
		THROW(("UUID mismatch for linked data in file %s", url.GetURL().c_str()));
	
	fMyFile.reset(new HBufferedFileStream(url, O_RDONLY | O_BINARY));
	fFile = fMyFile.get();
	
	fDecompressor.reset(CDecompressorImp::Create(url, *fFile, fKind,
		fDataOffset, fDataSize, fTableOffset, fTableSize));
}

string CLinkedDataImp::GetDocument(uint32 inDocNr)
{
	return fDecompressor->GetDocument(inDocNr);
}

void CLinkedDataImp::LinkData(const std::string& inDataFileName,
	const std::string& inDataFileUUID, HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize, int64& outTableOffset, int64& outTableSize)
{
	CDecompressorImp::LinkData(fDataFileName, fDataFileUUID, outData, outKind,
		outDataOffset, outDataSize, outTableOffset, outTableSize);
}

CDecompressorImp* CDecompressorImp::Create(const HUrl& inDb, HStreamBase& inFile, uint32 inKind,
	int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize)
{
	CDecompressorImp* result = NULL;
	
	switch (inKind)
	{
		case kHuffWordCompressed:
			result = new CHuffWordDecompressorImp(inFile, inDataOffset, inDataSize, inTableOffset, inTableSize);
			break;
		
		case kZLibCompressed:
			result = new CZLibDecompressorImp(inFile, inDataOffset, inDataSize, inTableOffset, inTableSize);
			break;
		
		case kbzLibCompressed:
			result = new CbzLibDecompressorImp(inFile, inDataOffset, inDataSize, inTableOffset, inTableSize);
			break;
		
		case kLinkedData:
			result = new CLinkedDataImp(inDb, inFile, inDataOffset, inDataSize, inTableOffset, inTableSize);
			break;
		
		default:
			THROW(("Invalid data file"));
	}

	result->Init();

	return result;
}

CDecompressor::CDecompressor(const HUrl& inDb,
		HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize,
		int64 inTableOffset, int64 inTableSize)
	: fImpl(CDecompressorImp::Create(inDb, inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize))
	, fKind(inKind)
{
}

CDecompressor::~CDecompressor()
{
	delete fImpl;
}

string CDecompressor::GetDocument(uint32 inDocNr)
{
	return fImpl->GetDocument(inDocNr);
}

void CDecompressor::CopyData(HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize)
{
	fImpl->CopyData(outData, outKind, outDataOffset, outDataSize, outTableOffset, outTableSize);
}

void CDecompressor::LinkData(const std::string& inDataFileName, const std::string& inDataFileUUID,
	HStreamBase& outData, uint32& outKind, int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize)
{
	fImpl->LinkData(inDataFileName, inDataFileUUID, outData, outKind,
		outDataOffset, outDataSize, outTableOffset, outTableSize);
}
