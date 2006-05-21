/*
	HStream.cpp
	
	Copyright Hekkelman Programmatuur b.v.
	
	Created: 09/15/97 03:53:14
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
 
#include "HLib.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "HStream.h"
#include "HFile.h"
#include "HFileCache.h"
#include "HError.h"
#include "HByteSwap.h"

#ifdef P_DEBUG
#include <iostream>
#endif

using namespace std;



HStreamBase::HStreamBase()
	: fSwap(false)
	, fBypass(false)
{
}

HStreamBase::~HStreamBase()
{
}

int32 HStreamBase::PWrite(const void* inBuffer, uint32 inSize, int64 inOffset)
{
	int64 cur_offset = Tell();
	Seek(inOffset, SEEK_SET);
	int32 result = Write(inBuffer, inSize);
	Seek(cur_offset, SEEK_SET);
	return result;
}

int32 HStreamBase::PRead(void* inBuffer, uint32 inSize, int64 inOffset)
{
	int64 cur_offset = Tell();
	Seek(inOffset, SEEK_SET);
	int32 result = Read(inBuffer, inSize);
	Seek(cur_offset, SEEK_SET);
	return result;
}

int64 HStreamBase::Tell() const
{
	HStreamBase* self = const_cast<HStreamBase*>(this);
	return self->Seek(0, SEEK_CUR);
}
	
int64 HStreamBase::Size() const
{
	int64 cur_offset = Tell();
	
	HStreamBase* self = const_cast<HStreamBase*>(this);
	
	int64 result = self->Seek(0, SEEK_END);
	self->Seek(cur_offset, SEEK_SET);
	return result;
}

void HStreamBase::Truncate(int64 /*inSize*/)
{
	THROW(("Truncate called on a stream object"));
}

void HStreamBase::SetSwapBytes(bool inSwap)
{
	fSwap = inSwap;
}

void HStreamBase::SetBypassCache(bool inBypass)
{
	fBypass = inBypass;
}

void HStreamBase::ReadString(string& outString)
{
	uint8 len;
	Read(&len, sizeof(len));
	char b[256];
	Read(b, len);
	outString.assign(b, len);
}

void HStreamBase::WriteString(string inString)
{
	ThrowIf(inString.length() > 255);
	uint8 len = inString.length();
	Write(&len, sizeof(len));
	Write(inString.c_str(), len);
}

HStreamBase& HStreamBase::operator>> (int8& d)
{
	Read(&d, sizeof(int8));
	return *this;
}

HStreamBase& HStreamBase::operator>> (int16& d)
{
	Read(&d, sizeof(int16));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (int32& d)
{
	Read(&d, sizeof(int32));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (float& d)
{
	Read(&d, sizeof(float));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (int64& d)
{
	Read(&d, sizeof(int64));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (uint8& d)
{
	Read(&d, sizeof(uint8));
	return *this;
}

HStreamBase& HStreamBase::operator>> (uint16& d)
{
	Read(&d, sizeof(uint16));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (uint32& d)
{
	Read(&d, sizeof(uint32));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (uint64& d)
{
	Read(&d, sizeof(uint64));
	if (fSwap)
		d = byte_swapper::swap(d);
	return *this;
}

HStreamBase& HStreamBase::operator>> (string& s)
{
	ReadString(s);
	return *this;
}

HStreamBase& HStreamBase::operator<< (int8 d)
{
	Write(&d, sizeof(int8));
	return *this;
}

HStreamBase& HStreamBase::operator<< (int16 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(int16));
	return *this;
}

HStreamBase& HStreamBase::operator<< (int32 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(int32));
	return *this;
}

HStreamBase& HStreamBase::operator<< (int64 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(int64));
	return *this;
}

HStreamBase& HStreamBase::operator<< (uint8 d)
{
	Write(&d, sizeof(uint8));
	return *this;
}

HStreamBase& HStreamBase::operator<< (uint16 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(uint16));
	return *this;
}

HStreamBase& HStreamBase::operator<< (uint32 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(uint32));
	return *this;
}

HStreamBase& HStreamBase::operator<< (float d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(float));
	return *this;
}

HStreamBase& HStreamBase::operator<< (uint64 d)
{
	if (fSwap)
		d = byte_swapper::swap(d);
	Write(&d, sizeof(uint64));
	return *this;
}

HStreamBase& HStreamBase::operator<< (const char* s)
{
	ThrowIfNil(s);
	WriteString(s);
	return *this;
}

HStreamBase& HStreamBase::operator<< (string s)
{
	WriteString(s);
	return *this;
}

#pragma mark memFullErr

#define kMemBlockSize 512

HMemoryStream::HMemoryStream()
{
	fPhysicalSize = kMemBlockSize;
	fData = new char [fPhysicalSize];
	if (fData == NULL) ThrowIfNil(fData);
	fLogicalSize = 0;
	fPointer = 0;
	fReadOnly = false;
}

HMemoryStream::HMemoryStream(void* inData, uint32 inSize)
{
	fPhysicalSize = fLogicalSize = inSize;
	fData = new char [fPhysicalSize];
	if (fData == NULL) ThrowIfNil(fData);
	memcpy(fData, inData, inSize);
	fPointer = 0;
	fReadOnly = false;
}

HMemoryStream::HMemoryStream(const void* inData, uint32 inSize)
{
	fPhysicalSize = fLogicalSize = inSize;
	fData = reinterpret_cast<char*>(const_cast<void*>(inData));
	if (fData == NULL) ThrowIfNil(fData);
	fPointer = 0;
	fReadOnly = true;
}

HMemoryStream::~HMemoryStream()
{
	if (not fReadOnly)
		delete [] fData;
}

int32 HMemoryStream::Read(void* inBuffer, uint32 inCount)
{
	if (fPointer > fLogicalSize or fData == nil)
		return -1;
	
	uint32 n = fLogicalSize - fPointer;
	if (n > inCount)
		n = inCount;
	
	memcpy(inBuffer, fData + fPointer, n);
	fPointer += n;
	
	return n;
}

int32 HMemoryStream::Write(const void* inBuffer, uint32 inCount)
{
	if (fReadOnly)
		THROW(("Error writing to a read-only memory stream"));
	
	assert (fPointer + inCount > 0);
	if (static_cast<unsigned int> (fPointer + inCount) > fPhysicalSize)
	{
		uint32 newSize = static_cast<uint32>(
			kMemBlockSize * ((fPointer + inCount) / kMemBlockSize + 1));
		
		char* t = new char[newSize];
		memcpy (t, fData, fPhysicalSize);
		delete [] fData;

		fData = t;
		fPhysicalSize = newSize;
	}
	
	memcpy(fData + fPointer, inBuffer,
		static_cast<uint32>(inCount));
		
	fPointer += inCount;
	if (fPointer >= fLogicalSize)
		fLogicalSize = fPointer;
	
	return inCount;
}

int64 HMemoryStream::Tell() const
{
	return fPointer;
}

int64 HMemoryStream::Seek(int64 inOffset, int inMode)
{
	switch (inMode)
	{
		case SEEK_SET:
			fPointer = inOffset;
			break;
		case SEEK_END:
			fPointer = static_cast<uint32>(fLogicalSize + inOffset);
			break;
		case SEEK_CUR:
			fPointer += inOffset;
			break;
		default:
			assert(false);
	}
	
	return fPointer;
}

int64 HMemoryStream::Size() const
{
	return static_cast<int64>(fLogicalSize);
}

const void* HMemoryStream::Buffer() const
{
	return fData;
}

void HMemoryStream::Truncate(int64 inOffset)
{
	if (fLogicalSize > inOffset)
		fLogicalSize = inOffset;
	if (fPointer > inOffset)
		fPointer = inOffset;
}

//HFileStream::HFileStream(int inFD)
//{
//	fFD = inFD;
//	ThrowIfPOSIXErr(fFD);
//}

HFileStream::HFileStream(const HUrl& inURL, int inMode)
	: fFD(0)
	, fOffset(0)
{
	ThrowIfPOSIXErr(fFD = HFile::Open(inURL, inMode));
	ThrowIfPOSIXErr(fSize = HFile::Seek(fFD, 0, SEEK_END));
	ThrowIfPOSIXErr(HFile::Seek(fFD, 0, SEEK_SET));
}

HFileStream::~HFileStream()
{
	Close();
}

int32 HFileStream::Read(void* inBuffer, uint32 inCount)
{
	int32 n = PRead(inBuffer, inCount, fOffset);

	ThrowIfPOSIXErr(n);
	
	if (n > 0)
		fOffset += n;
	
	return n;
}

int32 HFileStream::PRead(void* inBuffer, uint32 inSize, int64 inOffset)
{
	return HFile::PRead(fFD, inBuffer, inSize, inOffset);
}

int32 HFileStream::Write(const void* inBuffer, uint32 inSize)
{
	int32 n = PWrite(inBuffer, inSize, fOffset);

	ThrowIfPOSIXErr(n);

	if (n > 0)
		fOffset += n;

	return n;
}

int32 HFileStream::PWrite(const void* inBuffer, uint32 inSize, int64 inOffset)
{
	int32 result = HFile::PWrite(fFD, inBuffer, inSize, inOffset);
	ThrowIfPOSIXErr(result);

	if (result > 0 and inOffset + inSize > fSize)
		fSize = inOffset + inSize;
	
	return result;
}

int64 HFileStream::Tell() const
{
	return fOffset;
}

int64 HFileStream::Seek(int64 inOffset, int inMode)
{
	switch (inMode)
	{
		case SEEK_SET:
			fOffset = inOffset;
			break;
		case SEEK_CUR:
			fOffset += inOffset;
			break;
		case SEEK_END:
			fOffset = fSize;
			break;
		default:
			assert(false);
	}
	
	if (fOffset < 0)
		THROW(("Error in Seek"));

//	if (fOffset > fSize)
//		fSize = fOffset;
	
	return fOffset;
}

int64 HFileStream::Size() const
{
	return fSize;
}

void HFileStream::Truncate(int64 inSize)
{
	HFile::Truncate(fFD, inSize);
	
	fSize = inSize;
	
	if (fOffset > inSize)
		fOffset = inSize;
}

void HFileStream::Close()
{
	if (fFD >= 0)
		HFile::Close(fFD);
	fFD = -1;
}

//#if 0
HBufferedFileStream::HBufferedFileStream(const HUrl& inURL, int inMode)
	: HFileStream(inURL, inMode)
{
}

HBufferedFileStream::~HBufferedFileStream()
{
	Close();
}

int32 HBufferedFileStream::PWrite(const void* inBuffer, uint32 inSize, int64 inOffset)
{
	if (GetBypassCache())
		HFile::PWrite(fFD, inBuffer, inSize, inOffset);
	else
		HFileCache::Write(fFD, inBuffer, inSize, inOffset);
	
	if (inOffset + inSize > fSize)
		fSize = inOffset + inSize;
	
	return static_cast<int32>(inSize);
}

int32 HBufferedFileStream::PRead(void* inBuffer, uint32 inSize, int64 inOffset)
{
	int32 result;
	if (GetBypassCache())
		result = HFile::PRead(fFD, inBuffer, inSize, inOffset);
	else
		result = HFileCache::Read(fFD, inBuffer, inSize, inOffset);
	return result;
}

void HBufferedFileStream::Truncate(int64 inSize)
{
	HFileCache::Truncate(fFD, inSize);
	HFileStream::Truncate(inSize);
}

void HBufferedFileStream::Close()
{
	if (fFD >= 0)
	{
		HFileCache::Flush(fFD);
		HFileCache::Purge(fFD);
	}
	
	HFileStream::Close();
}

//#endif

HTempFileStream::HTempFileStream(const HUrl& inBaseName)
	: fTempUrl(new HUrl)
{
	fFD = HFile::CreateTempFile(inBaseName.GetParent(), inBaseName.GetFileName(), *fTempUrl);
	Truncate(0);
	fOffset = 0;
}

HTempFileStream::~HTempFileStream()
{
	Close();
}

void HTempFileStream::Close()
{
	HBufferedFileStream::Close();
	
	HFile::Unlink(*fTempUrl);
	delete fTempUrl;
}

// view

HStreamView::HStreamView(HStreamBase& inMain, int64 inOffset,
	int64 inSize)
	: fMain(inMain)
	, fOffset(inOffset)
	, fSize(inSize)
	, fPointer(0)
{
}

int32 HStreamView::Write(const void* inBuffer, uint32 inSize)
{
	assert(false);
	uint32 n = fMain.PWrite(inBuffer, inSize, fPointer + fOffset);
	fPointer += n;
	return n;
}

int32 HStreamView::Read(void* inBuffer, uint32 inSize)
{
	uint32 n = inSize;
	if (n > fSize - fPointer)
		n = fSize - fPointer;
	
	n = fMain.PRead(inBuffer, n, fPointer + fOffset);
	fPointer += n;
	return n;
}

int64 HStreamView::Seek(int64 inOffset, int inMode)
{
	switch (inMode)
	{
		case SEEK_CUR:
			fPointer += inOffset;
			break;
		case SEEK_SET:
			fPointer = inOffset;
			break;
		case SEEK_END:
			fPointer = fSize + inOffset;
			break;
	}
	
	if (fPointer > fSize)
		fPointer = fSize;
	else if (fPointer < 0)
		fPointer = 0;
	
	return fPointer;
}

///*
//	Buffered stream. Alternative implementation for HFileCache.
//*/
//
//const
//	kBufferBlockCount = 32,
//	kBufferBlockSize = 1024;
//
//HBufferedStream::HBufferedStream(HStreamBase* inStream, bool inOwner)
//	: fBlocks(new BufferBlock[kBufferBlockCount])
//	, fBlockCount(0)
//	, fStream(inStream)
//	, fOffset(0)
//	, fOwner(inOwner)
//{
//	assert(inStream);
//	ThrowIfNil(inStream);
//	fSize = inStream->Size();
//}
//
//HBufferedStream::HBufferedStream(HStreamBase& inStream)
//	: fBlocks(new BufferBlock[kBufferBlockCount])
//	, fBlockCount(0)
//	, fStream(&inStream)
//	, fSize(inStream.Size())
//	, fOffset(0)
//	, fOwner(false)
//{
//}
//
//HBufferedStream::~HBufferedStream()
//{
//	delete[] fBlocks;
//	
//	if (fOwner)
//		delete fStream;
//}
//
//int32 HBufferedStream::Write(const void* inBuffer, uint32 inSize)
//{
//	int32 w = PWrite(inBuffer, inSize, fOffset);
//	if (w > 0)
//	{
//		fOffset += w;
//		if (fOffset > fSize)
//			fSize = fOffset;
//	}
//	return w;
//}
//
//int32 HBufferedStream::PWrite(const void* inBuffer, uint32 inSize, int64 inOffset)
//{
//}
//
//int32 HBufferedStream::Read(void* inBuffer, uint32 inSize) = 0
//{
//	int32 r = PRead(inBuffer, inSize, fOffset);
//}
//
//int32 HBufferedStream::PRead(void* inBuffer, uint32 inSize, int64 inOffset)
//{
//}
//
//int64 HBufferedStream::Seek(int64 inOffset, int inMode) = 0
//{
//}
//
//int64 HBufferedStream::Tell() const
//{
//}
//
//int64 HBufferedStream::Size() const
//{
//}
//
//void HBufferedStream::Flush()
//{
//}
//

HStringStream::HStringStream(std::string inData)
	: fData(inData)
	, fOffset(0)
{
}

HStringStream::~HStringStream()
{
}

int32 HStringStream::Write(const void* /*inBuffer*/, uint32 /*inSize*/)
{
	assert(false);
	return -1;
}

int32 HStringStream::Read(void* inBuffer, uint32 inSize)
{
	int result = -1;
	
	if (fOffset < fData.length())
	{
		uint32 n = fData.length() - fOffset;
		if (n > inSize)
			n = inSize;
		
		if (n > 0)
			fData.copy(reinterpret_cast<char*>(inBuffer), n, fOffset);
		
		result = n;
		fOffset += n;
	}
	
	return result;
}

int64 HStringStream::Seek(int64 inOffset, int inMode)
{
	switch (inMode)
	{
		case SEEK_SET:
			fOffset = inOffset;
			break;
		case SEEK_END:
			fOffset = fData.length() + inOffset;
			break;
		case SEEK_CUR:
			fOffset += inOffset;
			break;
		default:
			assert(false);
	}
	
	if (fOffset > fData.length())
		fOffset = fData.length();
	
	return fOffset;
}
