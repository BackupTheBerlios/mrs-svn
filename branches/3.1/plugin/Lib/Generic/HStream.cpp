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
#include "HError.h"
#include "HByteSwap.h"
#include "HMutex.h"
#include "HUtils.h"

#ifdef P_DEBUG
#include <iostream>
#endif

using namespace std;



HStreamBase::HStreamBase()
	: fSwap(false)
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

void HStreamBase::CopyTo(HStreamBase& inDest, int64 inSize, int64 inOffset)
{
	Seek(inOffset, SEEK_SET);
	
	const int kBufSize = 64 * 4096;
	HAutoBuf<char> b(new char[kBufSize]);

	while (inSize > 0)
	{
		uint32 n = kBufSize;
		if (n > inSize)
			n = inSize;

		if (Read(b.get(), n) != int32(n))
			THROW(("I/O error in copy"));

		if (inDest.Write(b.get(), n) != int32(n))
			THROW(("I/O error in copy"));

		inSize -= n;
	}
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

#define kMemBlockSize			512
#define kMemBlockExtendSize		10240

HMemoryStream::HMemoryStream()
{
	fPhysicalSize = kMemBlockSize;
	fData = new char [fPhysicalSize];
	if (fData == NULL) ThrowIfNil(fData);
	fLogicalSize = 0;
	fPointer = 0;
	fReadOnly = false;
}

HMemoryStream::HMemoryStream(void* inData, int64 inSize)
{
	fPhysicalSize = fLogicalSize = inSize;
	fData = new char [fPhysicalSize];
	if (fData == NULL) ThrowIfNil(fData);
	memcpy(fData, inData, inSize);
	fPointer = 0;
	fReadOnly = false;
}

HMemoryStream::HMemoryStream(const void* inData, int64 inSize)
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
	
	int64 n = fLogicalSize - fPointer;
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
			kMemBlockExtendSize * ((fPointer + inCount) / kMemBlockExtendSize + 2));
		
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
			fPointer = static_cast<int64>(fLogicalSize + inOffset);
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

HFileStream::HFileStream(const HUrl& inURL, int inMode)
	: fFD(0)
	, fOffset(0)
	, fSize(0)
{
	ThrowIfPOSIXErr(fFD = HFile::Open(inURL, inMode));
	ThrowIfPOSIXErr(fSize = HFile::Seek(fFD, 0, SEEK_END));
	ThrowIfPOSIXErr(HFile::Seek(fFD, 0, SEEK_SET));
}

HFileStream::HFileStream()
	: fFD(0)
	, fOffset(0)
	, fSize(0)
{
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

/*
	Buffered stream. Alternative implementation for HFileCache.
*/

const uint32
	kBufferBlockCount = 64,
	kBufferBlockSize = 512;

HBufferedFileStream::HBufferedFileStream()
	: fBlocks(new BufferBlock[kBufferBlockCount])
	, fMutex(new HMutex)
{
	Init();
}

HBufferedFileStream::HBufferedFileStream(const HUrl& inURL, int inMode)
	: HFileStream(inURL, inMode)
	, fBlocks(new BufferBlock[kBufferBlockCount])
	, fMutex(new HMutex)
{
	Init();
}

HBufferedFileStream::~HBufferedFileStream()
{
	if (fFD >= 0)
		Close();
	
	for (uint32 ix = 0; ix < kBufferBlockCount; ++ix)
		delete[] fBlocks[ix].fText;

	delete[] fBlocks;
	fBlocks = nil;
	
	delete fMutex;
}

void HBufferedFileStream::Init()
{
	for (uint32 ix = 0; ix < kBufferBlockCount; ++ix)
	{
		fBlocks[ix].fText = new char[kBufferBlockSize];
		fBlocks[ix].fOffset = -1;
		fBlocks[ix].fPageSize = 0;
		fBlocks[ix].fDirty = false;
		fBlocks[ix].fNext = fBlocks + ix + 1;
		fBlocks[ix].fPrev = fBlocks + ix - 1;
	}
	
	fBlocks[0].fPrev = nil;
	fBlocks[kBufferBlockCount - 1].fNext = nil;
	
	fLRUHead = fBlocks;
	fLRUTail = fBlocks + kBufferBlockCount - 1;
}

void HBufferedFileStream::Close()
{
	if (fFD >= 0 and fBlocks != nil)
	{
		Flush();
		HFileStream::Close();
	}
}

void HBufferedFileStream::Flush()
{
	for (uint32 ix = 0; ix < kBufferBlockCount; ++ix)
	{
		if (fBlocks[ix].fDirty and fBlocks[ix].fOffset >= 0)
		{
			HFileStream::PWrite(fBlocks[ix].fText, fBlocks[ix].fPageSize, fBlocks[ix].fOffset);
			fBlocks[ix].fOffset = -1;
		}
	}
}

HBufferedFileStream::BufferBlock* HBufferedFileStream::GetBlock(int64 inOffset)
{
	BufferBlock* result = fLRUHead;
	uint32 n = 0;
	
	while (result != nil and result->fOffset != inOffset)
	{
		result = result->fNext;
		++n;
	}

	if (result == nil)
	{
		result = fLRUTail;
		
		if (result == nil)
			THROW(("HBufferedFileStream cache error"));
		
		if (result->fDirty and result->fOffset >= 0)
		{
			HFileStream::PWrite(result->fText, result->fPageSize, result->fOffset);
			result->fDirty = false;
		}
		
		int32 r = HFileStream::PRead(result->fText, kBufferBlockSize, inOffset);
		if (r < 0)
			THROW(("Error reading data from offset %Ld", inOffset));
		
		result->fDirty = false;
		result->fOffset = inOffset;
		result->fPageSize = r;
	}

	if (result != fLRUHead and n > (kBufferBlockCount / 4))
	{
		if (result == fLRUTail)
			fLRUTail = result->fPrev;
		
		if (result->fPrev)
			result->fPrev->fNext = result->fNext;
		if (result->fNext)
			result->fNext->fPrev = result->fPrev;

		result->fPrev = NULL;
		result->fNext = fLRUHead;
		fLRUHead->fPrev = result;
		fLRUHead = result;
	}
	
	return result;
}

int32 HBufferedFileStream::PWrite(const void* inBuffer, uint32 inSize, int64 inOffset)
{
	// short cut
	if (inSize > kBufferBlockCount * kBufferBlockSize)
	{
		StMutex lock(*fMutex);	// avoid corruption in cacheblocks due to multiple reads
		Flush();
		return HFileStream::PWrite(inBuffer, inSize, inOffset);
	}
	
	uint32 bOffset = 0;
	int32 rr = static_cast<int32>(inSize);	// for now...

	while (inSize > 0)
	{
		StMutex lock(*fMutex);	// avoid corruption in cacheblocks due to multiple reads

		int64 blockStart = (inOffset / kBufferBlockSize) * kBufferBlockSize;

		BufferBlock* e = GetBlock(blockStart);
		e->fDirty = true;

		if (e->fPageSize < kBufferBlockSize and 
			inOffset - blockStart + inSize > e->fPageSize)
		{
			e->fPageSize = static_cast<int32>(inOffset - blockStart + inSize);
			if (e->fPageSize > kBufferBlockSize)
				e->fPageSize = kBufferBlockSize;
		}

		uint32 size = static_cast<uint32>(e->fPageSize - (inOffset - blockStart));

		if (size > inSize)
			size = inSize;

		memcpy(e->fText + static_cast<uint32>(inOffset - blockStart),
			reinterpret_cast<const char*>(inBuffer) + bOffset, size);

		inSize -= size;
		inOffset += static_cast<long>(size);
		bOffset += size;
	}

	if (inOffset + inSize > fSize)
		fSize = inOffset + inSize;

	return rr;
}

int32 HBufferedFileStream::PRead(void* inBuffer, uint32 inSize, int64 inOffset)
{
	// short cut, bypass cache in case we're reading too much
	if (inSize > kBufferBlockCount * kBufferBlockSize)
	{
		StMutex lock(*fMutex);	// avoid corruption in cacheblocks due to multiple reads
		Flush();
		return HFileStream::PRead(inBuffer, inSize, inOffset);
	}
	
	uint32 bOffset = 0;
	int32 rr = 0;
	
	while (inSize > 0)
	{
		StMutex lock(*fMutex);	// avoid corruption in cacheblocks due to multiple reads

		int64 blockStart = (inOffset / kBufferBlockSize) * kBufferBlockSize;
		
		BufferBlock* e = GetBlock(blockStart);
		
		assert(e->fPageSize >= (inOffset - blockStart));

		uint32 size = static_cast<uint32>(e->fPageSize - (inOffset - blockStart));
		if (size > inSize)
			size = inSize;
		
		if (size == 0)
			break;

		rr += size;
		
		memcpy(reinterpret_cast<char*>(inBuffer) + bOffset,
			e->fText + static_cast<uint32>(inOffset - blockStart), size);

		inSize -= size;
		inOffset += size;
		bOffset += size;
	}
	
	return rr;
}

void HBufferedFileStream::Truncate(int64 inOffset)
{
	StMutex lock(*fMutex);	// avoid corruption in cacheblocks due to multiple reads

	for (uint32 ix = 0; ix < kBufferBlockCount; ++ix)
	{
		if (fBlocks[ix].fOffset > inOffset)
			fBlocks[ix].fOffset = -1;
		else
		{
			fBlocks[ix].fPageSize = static_cast<int32>(inOffset - fBlocks[ix].fOffset);

			if (fBlocks[ix].fPageSize > kBufferBlockSize)
				fBlocks[ix].fPageSize = kBufferBlockSize;

			if (fBlocks[ix].fPageSize > 0 and fBlocks[ix].fDirty and fBlocks[ix].fOffset >= 0)
			{
				HFileStream::PWrite(fBlocks[ix].fText, fBlocks[ix].fPageSize, fBlocks[ix].fOffset);
				fBlocks[ix].fDirty = false;
			}
		}
	}
	
	HFileStream::Truncate(inOffset);
}

HTempFileStream::HTempFileStream(const HUrl& inBaseName)
	: fTempUrl(new HUrl)
{
	fFD = HFile::CreateTempFile(inBaseName.GetParent(), inBaseName.GetFileName(), *fTempUrl);
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
	fTempUrl = nil;
}

// view

HStreamView::HStreamView(HStreamBase& inMain, int64 inOffset,
	int64 inSize)
	: fMain(inMain)
	, fOffset(inOffset)
	, fSize(inSize)
	, fPointer(0)
{
	SetSwapBytes(inMain.SwapsBytes());
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

