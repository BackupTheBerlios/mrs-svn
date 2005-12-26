/*	$Id: CBlastIndex.h,v 1.6 2005/10/11 13:17:30 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Friday May 27 2005 15:45:23
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
 
#ifndef CBLASTINDEX_H
#define CBLASTINDEX_H

#include "CSequence.h"
#include "CArray.h"

class CDatabank;

class CBlastIndex
{
  public:
						CBlastIndex(const HUrl& inDb);
						CBlastIndex(HStreamBase& inFile, uint32 inKind,
							int64 inDataOffset, int64 inDataSize,
							int64 inTableOffset, int64 inTableSize);
	virtual				~CBlastIndex();

	uint32				Count();
	uint32				CountSequencesForDocument(uint32 inDocNr);
	CSequence			GetSequence(uint32 inDocNr, uint32 inIndex);
	
	void				AddSequence(const CSequence& inSequence);
	void				AddSequence(const std::string& inSequence);

	void				FlushDoc();
	
	void				Finish(HStreamBase& inFile, int64& outDataOffset, int64& outDataSize,
							int64& outTableOffset, int64& outTableSize,
							uint32& outKind, uint32& outCount,
							int64& outSequenceLength, uint32& outSequenceCount);
	
	void				MergeIndices(std::vector<CDatabank*>& inParts);
	
  private:
	void				ExpandArray();

	CCArray<int64>*		fDocIndex;
	std::vector<CSequence>
						fSequences;
	HStreamBase*		fData;
	int64				fOffset;
	int64				fSize;
	uint32				fCount, fSequenceCount;
	int64				fTotalSequenceLength;
	HUrl				fOffsetsUrl;
	HStreamBase*		fBlastOffsets;
	HUrl				fDataUrl;
	HStreamBase*		fBlastData;
	const uint8*		fMappedBasePtr;
};

void
inline
CBlastIndex::AddSequence(const std::string& inSequence)
{
	AddSequence(Encode(inSequence));
}

#endif // CBLASTINDEX_H
