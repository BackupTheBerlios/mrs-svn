/*	$Id: CCompress.cpp 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Monday December 02 2002 15:31:25
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

#include <list>
#include <iostream>
#include <set>
#include "HFile.h"
#include "HError.h"
#include "HUtils.h"
#include "HStream.h"
#include <limits>

#include "zlib.h"
#include "bzlib.h"

#include "CCompress.h"
#include "CDatabank.h"
#include "CBitStream.h"
#include "CTextTable.h"

using namespace std;

const uint32
	kMaxDictionarySize = 0x7fff,
	kMaxMetaDataLength = numeric_limits<uint16>::max();

struct CCompressorImp
{
					CCompressorImp(
						uint32			inSig);

	virtual			~CCompressorImp();

	virtual void	CompressDocument(
						const char*		inText,
						uint32			inSize,
						HStreamBase&	inStream) = 0;
	
	virtual void	CompressData(
						vector<pair<const char*,uint32> >&
										inDataVector,
						HStreamBase&	inStream) = 0;

	virtual void	InitStream(
						HStreamBase&	inStream);
	
	static CCompressorImp*
					Create(
						const uint32	inCompressionKind,
						int32			inCompressionLevel,
						const string&	inDictionary);
	
	uint32			sig;
};

CCompressorImp::CCompressorImp(
	uint32				inSig)
	: sig(inSig)
{
}

CCompressorImp::~CCompressorImp()
{
}

void CCompressorImp::InitStream(
	HStreamBase&		inStream)
{
}

namespace {

// ------------------------------------------------------------------
//
//	The ZLib compressor impl
//

struct CZLibCompressorImp : public CCompressorImp
{
					CZLibCompressorImp(
						int32			inCompressionLevel,
						const string&	inDictionary);

					~CZLibCompressorImp();
	
	virtual void	CompressDocument(
						const char*		inData,
						uint32			inDataLength,
						HStreamBase&	inStream);
	
	virtual void	CompressData(
						vector<pair<const char*,uint32> >&
										inDataVector,
						HStreamBase&	inStream);

	virtual void	InitStream(
						HStreamBase&	inStream);
	
	z_stream_s		z_stream;
	string			dictionary;
};

CZLibCompressorImp::CZLibCompressorImp(
	int32			inCompressionLevel,
	const string&	inDictionary)
	: CCompressorImp(kZLibCompressed)
{
	memset(&z_stream, 0, sizeof(z_stream));

	int err = deflateInit(&z_stream, inCompressionLevel);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	// write a simplified dictionary
	dictionary = inDictionary;
	if (dictionary.length() > kMaxDictionarySize)
		dictionary.erase(0, dictionary.length() - kMaxDictionarySize);
}

CZLibCompressorImp::~CZLibCompressorImp()
{
	deflateEnd(&z_stream);
}

void CZLibCompressorImp::InitStream(
	HStreamBase&	inStream)
{
	HSwapStream<net_swapper> data(inStream);
	
	uint16 ds = dictionary.length();
	data << ds;

	if (ds > 0)
		data.Write(dictionary.c_str(), ds);
}

void CZLibCompressorImp::CompressDocument(
	const char*		inText,
	uint32			inSize,
	HStreamBase&	inStream)
{
	HSwapStream<net_swapper> data(inStream);
	
	const int kBufferSize = 4096;
	static HAutoBuf<unsigned char> sBuffer(new unsigned char[kBufferSize]);

	z_stream.next_in = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(inText));
	z_stream.avail_in = inSize;
	z_stream.total_in = 0;
	
	z_stream.next_out = sBuffer.get();
	z_stream.avail_out = kBufferSize;
	z_stream.total_out = 0;
	
	int err = deflateReset(&z_stream);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	if (dictionary.length() > 0)
	{
		err = deflateSetDictionary(&z_stream,
			reinterpret_cast<const unsigned char*>(dictionary.c_str()), dictionary.length());
		if (err != Z_OK)
			THROW(("Compressor error: %s", z_stream.msg));
	}

	for (;;)
	{
		err = deflate(&z_stream, Z_FINISH);
		
		if (z_stream.total_out > 0)
			data.Write(sBuffer.get(), z_stream.total_out);
		
		if (err == Z_OK)
		{
			z_stream.next_out = sBuffer.get();
			z_stream.avail_out = kBufferSize;
			z_stream.total_out = 0;
			continue;
		}
		
		break;
	}
	
	if (err != Z_STREAM_END)
		THROW(("Deflate error: %s (%d)", z_stream.msg, err));
}

void CZLibCompressorImp::CompressData(
	vector<pair<const char*,uint32> >&
					inDataVector,
	HStreamBase&	inStream)
{
	HSwapStream<net_swapper> data(inStream);

	for (uint32 ix = 1; ix < inDataVector.size(); ++ix)
		data << static_cast<uint16>(inDataVector[ix - 1].second);
	
	const uint32 kBufferSize = 4096;
	static HAutoBuf<unsigned char> sBuffer(new unsigned char[kBufferSize]);

	z_stream.avail_in = 0;
	z_stream.total_in = 0;
	
	z_stream.next_out = sBuffer.get();
	z_stream.avail_out = kBufferSize;
	z_stream.total_out = 0;
	
	int err = deflateReset(&z_stream);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	if (dictionary.length() > 0)
	{
		err = deflateSetDictionary(&z_stream,
			reinterpret_cast<const unsigned char*>(dictionary.c_str()), dictionary.length());
		if (err != Z_OK)
			THROW(("Compressor error: %s", z_stream.msg));
	}

	int action = Z_NO_FLUSH;
	uint32 dvIx = 0;

	for (;;)
	{
		if (z_stream.avail_in == 0)
		{
			if (dvIx < inDataVector.size())
			{
				z_stream.next_in = const_cast<unsigned char*>(
					reinterpret_cast<const unsigned char*>(inDataVector[dvIx].first));
				z_stream.avail_in = inDataVector[dvIx].second;
				++dvIx;
				continue;	// in case this field was empty...
			}
			else
				action = Z_FINISH;
		}
		
		err = deflate(&z_stream, action);
		
		if (z_stream.avail_out < kBufferSize)
			data.Write(sBuffer.get(), kBufferSize - z_stream.avail_out);
		
		if (err == Z_OK)
		{
			z_stream.next_out = sBuffer.get();
			z_stream.avail_out = kBufferSize;
			continue;
		}
		
		break;
	}
	
	if (err != Z_STREAM_END)
		THROW(("Deflate error: %s (%d)", z_stream.msg, err));
}

// ------------------------------------------------------------------
//
//	The bzip2 compressor impl
//

struct CbzLibCompressorImp : public CCompressorImp
{
					CbzLibCompressorImp(
						int32			inCompressionLevel);

					~CbzLibCompressorImp();

	virtual void	CompressDocument(
						const char*		inText,
						uint32			inSize,
						HStreamBase&	inStream);
	
	virtual void	CompressData(
						vector<pair<const char*,uint32> >&
										inDataVector,
						HStreamBase&	inStream);
	
	bz_stream		fStream;
	int				fCompressionLevel;
};

CbzLibCompressorImp::CbzLibCompressorImp(
	int32			inCompressionLevel)
	: CCompressorImp(kbzLibCompressed)
	, fCompressionLevel(inCompressionLevel)
{
	memset(&fStream, 0, sizeof(fStream));
}

CbzLibCompressorImp::~CbzLibCompressorImp()
{
}

void CbzLibCompressorImp::CompressDocument(
	const char*		inText,
	uint32			inSize,
	HStreamBase&	inStream)
{
	HSwapStream<net_swapper> data(inStream);
	
	int err = BZ2_bzCompressInit(&fStream, fCompressionLevel, 0, 0);
	if (err != BZ_OK)
		THROW(("Compressor error: %d", err));

	const int kBufferSize = 4096;
	static HAutoBuf<char> sBuffer(new char[kBufferSize]);

	fStream.next_in = const_cast<char*>(inText);
	fStream.avail_in = inSize;
	fStream.total_in_lo32 = 0;
	fStream.total_in_hi32 = 0;
	
	fStream.next_out = sBuffer.get();
	fStream.avail_out = kBufferSize;
	fStream.total_out_lo32 = 0;
	fStream.total_out_hi32 = 0;
	
	int action = BZ_RUN;
	
	for (;;)
	{
		err = BZ2_bzCompress(&fStream, action);
		action = BZ_FINISH;
		
		if (fStream.total_out_lo32 > 0)
			data.Write(sBuffer.get(), fStream.total_out_lo32);
		
		if (err == BZ_FINISH_OK or err == BZ_RUN_OK)
		{
			fStream.next_out = sBuffer.get();
			fStream.avail_out = kBufferSize;
			fStream.total_out_lo32 = 0;
			continue;
		}
		
		break;
	}
	
	if (err != BZ_STREAM_END)
		THROW(("bzCompress error: %d", err));

	BZ2_bzCompressEnd(&fStream);
	memset(&fStream, 0, sizeof(fStream));
}

void CbzLibCompressorImp::CompressData(
	vector<pair<const char*,uint32> >&
					inDataVector,
	HStreamBase&	inStream)
{
	HSwapStream<net_swapper> data(inStream);
	
	for (uint32 ix = 1; ix < inDataVector.size(); ++ix)
		data << static_cast<uint16>(inDataVector[ix - 1].second);
	
	int err = BZ2_bzCompressInit(&fStream, fCompressionLevel, 0, 0);
	if (err != BZ_OK)
		THROW(("Compressor error: %d", err));

	const int kBufferSize = 4096;
	static HAutoBuf<char> sBuffer(new char[kBufferSize]);

	fStream.next_in = const_cast<char*>(inDataVector[0].first);
	fStream.avail_in = inDataVector[0].second;
	fStream.total_in_lo32 = 0;
	fStream.total_in_hi32 = 0;
	
	fStream.next_out = sBuffer.get();
	fStream.avail_out = kBufferSize;
	fStream.total_out_lo32 = 0;
	fStream.total_out_hi32 = 0;
	
	int action = BZ_RUN;
	uint32 dvIx = 1;
	
	for (;;)
	{
		err = BZ2_bzCompress(&fStream, action);
		
		if (fStream.total_out_lo32 > 0)
			data.Write(sBuffer.get(), fStream.total_out_lo32);
		
		if (err == BZ_FINISH_OK or err == BZ_RUN_OK)
		{
			fStream.next_out = sBuffer.get();
			fStream.avail_out = kBufferSize;
			fStream.total_out_lo32 = 0;
			
			if (fStream.avail_in == 0)
			{
				if (dvIx < inDataVector.size())
				{
					fStream.next_in = const_cast<char*>(inDataVector[dvIx].first);
					fStream.avail_in = inDataVector[dvIx].second;
					++dvIx;
				}
				else
					action = BZ_FINISH;
			}
			
			continue;
		}
		
		break;
	}
	
	if (err != BZ_STREAM_END)
		THROW(("bzCompress error: %d", err));

	BZ2_bzCompressEnd(&fStream);
	memset(&fStream, 0, sizeof(fStream));
}

}

CCompressorImp* CCompressorImp::Create(
	uint32			inCompressionType,
	int32			inCompressionLevel,
	const string&	inDictionary)
{
	CCompressorImp* result = NULL;
	
	switch (inCompressionType)
	{
		case kZLibCompressed:
			result = new CZLibCompressorImp(inCompressionLevel, inDictionary);
			break;
		
		case kbzLibCompressed:
			result = new CbzLibCompressorImp(inCompressionLevel);
			break;
		
		default:
			THROW(("Unknown compression type"));
	}

	return result;
}

// ------------------------------------------------------------------
//
//	The compressor factory
//

CCompressorFactory::CCompressorFactory(
	const string&	inCompressionAlgorithmName,
	int32			inCompressionLevel,
	const string&	inDictionary)
	: fKind(kInvalidData)
	, fLevel(inCompressionLevel)
	, fDictionary(inDictionary)
{
	if (inCompressionAlgorithmName == "huffword")
		THROW(("huffword is no longer supported as compression algorithm"));
	else if (inCompressionAlgorithmName == "zlib" or
			 inCompressionAlgorithmName == "gzip")
	{
		fKind = kZLibCompressed;
		if (VERBOSE)
			cout << "Using zlib compression" << endl;
	}
	else if (inCompressionAlgorithmName == "bzip2" or
			 inCompressionAlgorithmName == "bzip")
	{
		fKind = kbzLibCompressed;
		if (VERBOSE)
			cout << "Using bzip compression" << endl;
	}
	else
		THROW(("Unknown compression algorithm"));
}

void CCompressorFactory::InitDataStream(
	HStreamBase&	outData)
{
	auto_ptr<CCompressorImp> imp(CCompressorImp::Create(fKind, fLevel, fDictionary));
	imp->InitStream(outData);
}

CCompressor* CCompressorFactory::CreateCompressor()
{
	return new CCompressor(CCompressorImp::Create(fKind, fLevel, fDictionary));
}

// ------------------------------------------------------------------
//
//	The compressor base class
//

CCompressor::CCompressor(
	CCompressorImp*		inImpl)
	: fImpl(inImpl)
{
}

CCompressor::~CCompressor()
{
	delete fImpl;
}

void CCompressor::CompressDocument(
	const char*		inText,
	uint32			inSize,
	HStreamBase&	inStream)
{
	assert(inSize > 0);

	fImpl->CompressDocument(inText, inSize, inStream);
}

void CCompressor::CompressData(
	vector<pair<const char*,uint32> >&
					inDataVector,
	HStreamBase&	inStream)
{
	assert(inDataVector.size() > 1);

	fImpl->CompressData(inDataVector, inStream);
}

