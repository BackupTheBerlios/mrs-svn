/*	$Id: CDecompress.cpp 331 2007-02-12 07:44:10Z hekkel $
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
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);
	virtual			~CDecompressorImp();
	
	virtual string	GetDocument(uint32 inDocNr) = 0;
	virtual string	GetField(uint32 inDocNr, uint32 inFieldNr) = 0;

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
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);

	virtual void	Init();
	
	HStreamBase*	fFile_;
	HStreamView		fData;
	uint32			fKind;
	int64			fDataOffset, fDataSize;
	int64			fTableOffset, fTableSize;
	uint32			fMetaDataCount, fPrefixLength;
};

CDecompressorImp::CDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize,
		uint32 inMetaDataCount)
	: fFile_(&inFile)
	, fData(inFile, inDataOffset, inDataSize)
	, fKind(inKind)
	, fDataOffset(inDataOffset)
	, fDataSize(inDataSize)
	, fTableOffset(inTableOffset)
	, fTableSize(inTableSize)
	, fMetaDataCount(inMetaDataCount)
	, fPrefixLength(0)
{
}

CDecompressorImp::~CDecompressorImp()
{
}

void CDecompressorImp::Init()
{
}

void CDecompressorImp::LinkData(const std::string& inDataFileName,
	const std::string& inDataFileUUID, HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize, int64& outTableOffset, int64& outTableSize)
{
	outDataOffset = outData.Seek(0, SEEK_END);
	
	outData << inDataFileName << inDataFileUUID
		 << fKind
		 << fDataOffset << fDataSize
		 << fTableOffset << fTableSize;
	
	outDataSize = outData.Tell() - outDataOffset;
	outTableOffset = 0;
	outTableSize = 0;
	
	outKind = kLinkedDataV2;
}

void CDecompressorImp::CopyData(HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize)
{
	outKind = fKind;
	outDataOffset = outData.Seek(0, SEEK_END);
	outDataSize = fDataSize;
	
	fFile_->CopyTo(outData, fDataSize, fDataOffset);

	outTableOffset = outData.Tell();
	outTableSize = fTableSize;
	
	fFile_->CopyTo(outData, fTableSize, fTableOffset);
}

struct CBasicDecompressorImp : public CDecompressorImp
{
					CBasicDecompressorImp(HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);
	virtual			~CBasicDecompressorImp();

	virtual void	Init();

	virtual string	GetDocument(uint32 inDocNr);
	virtual string	GetField(uint32 inDocNr, uint32 inFieldNr);

	virtual string	GetDocument(int64 inOffset, uint32 inSize) = 0;

	CCArray<int64>*	fDocIndex;
};

CBasicDecompressorImp::CBasicDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize, uint32 inMetaDataCount)
	: CDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount)
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
	
	fDocIndex = new CCArray<int64>(*fFile_, fTableOffset, fTableSize, fDataSize);
}

string CBasicDecompressorImp::GetDocument(uint32 inDocNr)
{
	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inDocNr];
	size = static_cast<uint32>((*fDocIndex)[inDocNr + 1] - offset);

	assert(offset + size <= fDataSize);

	return GetDocument(fPrefixLength + offset, size);
}

string CBasicDecompressorImp::GetField(uint32 inDocNr, uint32 inFieldNr)
{
	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inDocNr];
	size = static_cast<uint32>((*fDocIndex)[inDocNr + 1] - offset);

	assert(offset + size <= fDataSize);

	fData.Seek(fPrefixLength + offset, SEEK_SET);
	
	uint32 offsetTableLength = sizeof(uint16) * fMetaDataCount;
	
	HAutoBuf<uint16> offsets(new uint16[fMetaDataCount + 1]);

	offsets[0] = 0;
	for (uint32 ix = 1; ix <= fMetaDataCount; ++ix)
	{
		uint16 n;
		fData >> n;
		offsets[ix] = offsets[ix - 1] + n;
	}
	
	string result = GetDocument(fPrefixLength + offset + offsetTableLength, size - offsetTableLength);
	
	if (inFieldNr < fMetaDataCount)
		result = result.substr(offsets[inFieldNr], offsets[inFieldNr + 1] - offsets[inFieldNr]);
	else
		result = result.substr(offsets[fMetaDataCount]);
	
	return result;
}

// compression using zlib

struct CZLibDecompressorImp : public CBasicDecompressorImp
{
					CZLibDecompressorImp(HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);
	virtual			~CZLibDecompressorImp();
	
	virtual void	Init();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);

	z_stream_s		fZStream;
	unsigned char*	fDictionary;
	uint32			fDictLength;
};

CZLibDecompressorImp::CZLibDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize, uint32 inMetaDataCount)
	: CBasicDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount)
	, fDictionary(NULL)
	, fDictLength(0)
{
#if P_LITTLEENDIAN
	if (inKind == kZLibCompressedV1)
		fData.SetSwapBytes(true);
#endif
}

CZLibDecompressorImp::~CZLibDecompressorImp()
{
	delete fDictionary;
	inflateEnd(&fZStream);
}

void CZLibDecompressorImp::Init()
{
	CBasicDecompressorImp::Init();

	// read fDictionary
	uint16 n;
	fData >> n;

	if (n > 0)
	{
		fDictLength = n;
		fDictionary = new unsigned char[fDictLength];
	
		fData.Read(fDictionary, fDictLength);
	}
	
	fPrefixLength = sizeof(n) + n;
	
	memset(&fZStream, 0, sizeof(fZStream));

	int err = inflateInit(&fZStream);
	if (err != Z_OK)
		THROW(("Decompression error: %s", fZStream.msg));
}
	
string CZLibDecompressorImp::GetDocument(int64 inOffset, uint32 inLength)
{
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

	while (not done and err >= Z_OK)
	{
			// shift remaining unread bytes to the beginning of our buffer
		if (fZStream.next_in > src_buf.get() and fZStream.avail_in > 0)
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

			inOffset += fData.PRead(src_buf.get() + fZStream.avail_in, size, inOffset);
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
					CbzLibDecompressorImp(HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);
	virtual			~CbzLibDecompressorImp();
	
	virtual string	GetDocument(int64 inOffset, uint32 inSize);
};

CbzLibDecompressorImp::CbzLibDecompressorImp(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize, uint32 inMetaDataCount)
	: CBasicDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount)
{
#if P_LITTLEENDIAN
	if (inKind == kbzLibCompressedV1)
		fData.SetSwapBytes(true);
#endif
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
	while (not done and err >= Z_OK)
	{
			// shift remaining unread bytes to the beginning of our buffer
		if (stream.next_in > src_buf.get() and stream.avail_in > 0)
			memmove(src_buf.get(), stream.next_in, stream.avail_in);

		stream.next_in = src_buf.get();

			// flush parameter, if we still have data set it to 0
		if (inLength > 0)
		{
			uint32 size = kBufferSize - stream.avail_in;
			if (size > inLength)
				size = static_cast<uint32>(inLength);
			inLength -= size;

			inOffset += fData.PRead(src_buf.get() + stream.avail_in, size, inOffset);
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

class CLinkedDataImp : public CDecompressorImp
{
  public:
					CLinkedDataImp(const HUrl& inURL, HStreamBase& inFile, uint32 inKind,
						int64 inDataOffset, int64 inDataSize,
						int64 inTableOffset, int64 inTableSize,
						uint32 inMetaDataCount);
	
	virtual string	GetDocument(uint32 inDocNr);
	virtual string	GetField(uint32 inDocNr, uint32 inFieldNr);

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

CLinkedDataImp::CLinkedDataImp(const HUrl& inURL, HStreamBase& inFile, uint32 inKind,
	int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize, uint32 inMetaDataCount)
	: CDecompressorImp(inFile, 0, 0, 0, 0, 0, inMetaDataCount)
{
#if P_LITTLEENDIAN
	if (inKind == kLinkedDataV1)
		fData.SetSwapBytes(true);
#endif
	
	fData >> fDataFileName >> fDataFileUUID
		 >> fKind
		 >> fDataOffset >> fDataSize
		 >> fTableOffset >> fTableSize;
	
	HUrl dir(inURL.GetParent());
	HUrl url(dir.GetChild(fDataFileName));
	
	if (not HFile::Exists(url))
		THROW(("Linked data file (%s) not found", url.GetURL().c_str()));
	
	CDatabank db(url);
	
	if (db.GetUUID() != fDataFileUUID)
		THROW(("UUID mismatch for linked data in file %s", url.GetURL().c_str()));
	
	fMyFile.reset(new HBufferedFileStream(url, O_RDONLY | O_BINARY));
	fFile_ = fMyFile.get();
	
	fDecompressor.reset(CDecompressorImp::Create(url, *fFile_, fKind,
		fDataOffset, fDataSize, fTableOffset, fTableSize, inMetaDataCount));
}

string CLinkedDataImp::GetDocument(uint32 inDocNr)
{
	return fDecompressor->GetDocument(inDocNr);
}

string CLinkedDataImp::GetField(uint32 inDocNr, uint32 inFieldNr)
{
	return fDecompressor->GetField(inDocNr, inFieldNr);
}

void CLinkedDataImp::LinkData(const std::string& inDataFileName,
	const std::string& inDataFileUUID, HStreamBase& outData, uint32& outKind,
	int64& outDataOffset, int64& outDataSize, int64& outTableOffset, int64& outTableSize)
{
	CDecompressorImp::LinkData(fDataFileName, fDataFileUUID, outData, outKind,
		outDataOffset, outDataSize, outTableOffset, outTableSize);
}

CDecompressorImp* CDecompressorImp::Create(const HUrl& inDb, HStreamBase& inFile, uint32 inKind,
	int64 inDataOffset, int64 inDataSize, int64 inTableOffset, int64 inTableSize, uint32 inMetaDataCount)
{
	CDecompressorImp* result = NULL;
	
	switch (inKind)
	{
		case kZLibCompressedV1:
		case kZLibCompressedV2:
			result = new CZLibDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount);
			break;
		
		case kbzLibCompressedV1:
		case kbzLibCompressedV2:
			result = new CbzLibDecompressorImp(inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount);
			break;
		
		case kLinkedDataV1:
		case kLinkedDataV2:
			result = new CLinkedDataImp(inDb, inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount);
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
		int64 inTableOffset, int64 inTableSize,
		uint32 inMetaDataCount)
	: fImpl(CDecompressorImp::Create(inDb, inFile, inKind, inDataOffset, inDataSize, inTableOffset, inTableSize, inMetaDataCount))
	, fKind(inKind)
	, fMetaDataCount(inMetaDataCount)
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

string CDecompressor::GetField(uint32 inEntry, uint32 inIndex)
{
	return fImpl->GetField(inEntry, inIndex);
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
