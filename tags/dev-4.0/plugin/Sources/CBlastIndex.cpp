/*	$Id: CBlastIndex.cpp 315 2007-02-06 15:28:39Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Friday May 27 2005 15:45:56
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

#include <zlib.h>

#include "HUtils.h"
#include "HUrl.h"
#include "HFile.h"
#include <vector>
#include <iomanip>

#include "CBlastIndex.h"
#include "CBitStream.h"
#include "CDatabank.h"

using namespace std;

const uint32
	kBlastCompressionLevel = 9,
	kBlastIndexProtein	= FOUR_CHAR_INLINE('prot');

CBlastIndex::CBlastIndex(const HUrl& inDb)
	: fDocIndex(NULL)
	, fData(NULL)
	, fOffset(0)
	, fSize(0)
	, fCount(0)
	, fSequenceCount(0)
	, fTotalSequenceLength(0)
	, fBlastOffsets(nil)
	, fBlastData(nil)
{
	fOffsetsUrl = HUrl(inDb.GetURL() + ".blast_offsets");
	fBlastOffsets = new HTempFileStream(fOffsetsUrl);

	fDataUrl = HUrl(inDb.GetURL() + ".blast_data");
	fBlastData = new HTempFileStream(fDataUrl);

	// now init the table
	(*fBlastOffsets) << fBlastData->Tell();
}

CBlastIndex::CBlastIndex(HStreamBase& inFile, uint32 inKind,
		int64 inDataOffset, int64 inDataSize,
		int64 inTableOffset, int64 inTableSize)
	: fDocIndex(NULL)
	, fData(&inFile)
	, fOffset(inDataOffset)
	, fSize(inDataSize)
	, fCount(0)
	, fBlastOffsets(nil)
	, fBlastData(nil)
{
	fDocIndex = new CCArray<int64>(*fData, inTableOffset, inTableSize, inDataSize);
}

CBlastIndex::~CBlastIndex()
{
	delete fDocIndex;
	delete fBlastOffsets;
	delete fBlastData;
}

uint32 CBlastIndex::Count()
{
	return fCount;
}

void CBlastIndex::ExpandArray()
{
	fDocIndex->Expand();
	
	fMappedBasePtr = nil;

	try
	{
		HMMappedFileStream* mmappedData =
			new HMMappedFileStream(dynamic_cast<HFileStream&>(*fData), fOffset, fSize);
		
		fBlastData = mmappedData;
		fMappedBasePtr = static_cast<const uint8*>(mmappedData->Buffer());
		
		if (VERBOSE)
			cerr << "Using memory mapped blast index" << endl;
	}
	catch (std::exception& inErr)
	{
		DisplayError(inErr);

		fBlastData = new HStreamView(*fData, fOffset, fSize);

		if (VERBOSE)
			cerr << "Using plain file access for blast" << endl;
	}
}

uint32 CBlastIndex::CountSequencesForDocument(uint32 inDocNr)
{
	if (not fDocIndex->IsExpanded())
		ExpandArray();

	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inDocNr];
	size = static_cast<uint32>((*fDocIndex)[inDocNr + 1] - offset);

	HStreamView data(*fBlastData, offset, size);
#if P_LITTLEENDIAN
	data.SetSwapBytes(true);
#endif
	
	uint32 count;
	data >> count;

	return count;
}

// the next function should be thread safe. And it is, if you call it once
// before you fire the threads. That's not very clean, I know.

// •••• NO, it isn't thread safe.... duh...
CSequence CBlastIndex::GetSequence(uint32 inDocNr, uint32 inIndex)
{
	if (not fDocIndex->IsExpanded())
		ExpandArray();
	
	int64 offset;
	uint32 size;

	offset = (*fDocIndex)[inDocNr];
	size = static_cast<uint32>((*fDocIndex)[inDocNr + 1] - offset);
	
	// use a wrapper here to make this function thread safe
	HStreamView data(*fBlastData, offset, size);
//	HMemoryStream data(fMappedBasePtr + offset, size);
#if P_LITTLEENDIAN
	data.SetSwapBytes(true);
#endif

	uint32 count;
	data >> count;

	assert(inIndex < count);
	if (inIndex >= count)
		THROW(("Index %d for sequence is out of range", inIndex));

//	HAutoBuf<uint32> offsets(new uint32[count + 1]);		<-- this appears to take too much time
	const int kOffsetBufSize = 5;

	uint32 offsetBuf[kOffsetBufSize];
	uint32* offsets = offsetBuf;
	if (count >= kOffsetBufSize)
		offsets = new uint32[count + 1];
	
	for (uint32 i = 0; i <= count; ++i)
		data >> offsets[i];
	
	offset += offsets[inIndex];
	size = offsets[inIndex + 1] - offsets[inIndex];
	
	if (count >= kOffsetBufSize)
		delete[] offsets;
	
	if (fMappedBasePtr != nil)
	{
		return CSequence(fMappedBasePtr + offset, size);
	}
	else
	{
		CSequence result(size);
		fBlastData->PRead(const_cast<uint8*>(result.c_str()), size, offset);
		return result;
	}
}

void CBlastIndex::AddSequence(const CSequence& inSequence)
{
	fSequences.push_back(inSequence);
	++fSequenceCount;
	fTotalSequenceLength += inSequence.length();
}

void CBlastIndex::FlushDoc()
{
	uint32 count = fSequences.size();

//	fBlastData->Seek(offset, SEEK_SET);
	*fBlastData << count;
	
	uint32 offset = (count + 2) * sizeof(uint32);
	for (uint32 i = 0; i < count; ++i)
	{
		*fBlastData << offset;
		offset += fSequences[i].length();
	}
	*fBlastData << offset;
	
	for (uint32 i = 0; i < count; ++i)
		fBlastData->Write(fSequences[i].c_str(), fSequences[i].length());

	fSequences.clear();
	
	(*fBlastOffsets) << fBlastData->Tell();
	++fCount;
}

void CBlastIndex::Finish(HStreamBase& inFile, int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize, uint32& outKind, uint32& outCount,
	int64& outSequenceLength, uint32& outSequenceCount)
{
	const uint32 kCopyBufferSize = 4 * 1024 * 1024;
	
	outKind = kBlastIndexProtein;
	outCount = fCount;
	
	outDataOffset = inFile.Tell();
	outDataSize = fBlastData->Tell();

	HAutoBuf<char> b(new char[kCopyBufferSize]);

	int64 size = outDataSize;
	fBlastData->Seek(0, SEEK_SET);
	
	while (size > 0)
	{
		int64 n = size;
		if (n > kCopyBufferSize)
			n = kCopyBufferSize;
		
		fBlastData->Read(b.get(), n);
		inFile.Write(b.get(), n);
		
		size -= n;
	}
	
	outTableOffset = inFile.Tell();
	CCArray<int64> arr(*fBlastOffsets, outDataSize);
	inFile.Write(arr.Peek(), arr.Size());
	outTableSize = inFile.Tell() - outTableOffset;
	
	outSequenceLength = fTotalSequenceLength;
	outSequenceCount = fSequenceCount;
}

void CBlastIndex::MergeIndices(vector<CDatabank*>& inParts)
{
	// yes, this can be improved, I know...
	
	if (VERBOSE)
		cerr << "Merging blast indices" << endl;
	
	for (vector<CDatabank*>::iterator p = inParts.begin(); p != inParts.end(); ++p)
	{
		CBlastIndex* ix = (*p)->GetBlastIndex();
		
		if (ix == nil)
		{
			if (VERBOSE)
				cerr << "No blast indices found, skipping" << endl;
			return;
		}
		
		for (uint32 d = 0; d < (*p)->Count(); ++d)
		{
			for (uint32 s = 0; s < ix->CountSequencesForDocument(d); ++s)
				AddSequence(ix->GetSequence(d, s));
			
			FlushDoc();
		}
	}
	
	if (VERBOSE)
		cerr << " done" << endl;
}
