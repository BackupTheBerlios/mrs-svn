/*	$Id: CDatabank.cpp 21 2006-03-09 13:41:27Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

/*-
 * Copyright (c) 2005
 *      M.L. Hekkelman. All rights reserved.
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

#include "HFile.h"
#include "HUrl.h"
#include "HStream.h"
#include "HError.h"
 
#include "CDocWeightArray.h"

// ----------------------------------------------------------------------------
//
//	DocWeightArray support
//

struct CDocWeightArrayImp
{
						CDocWeightArrayImp(uint32 inCount)
							: fCount(inCount)
							, fRefCount(1)
						{
						}

	virtual				~CDocWeightArrayImp() {}

	void				Reference();
	void				Release();

	virtual float		at(uint32 inIndex) const = 0;

	uint32				fCount;
	uint32				fRefCount;
};

void CDocWeightArrayImp::Reference()
{
	++fRefCount;
}

void CDocWeightArrayImp::Release()
{
	if (--fRefCount == 0)
		delete this;
}

struct CSimpleDocWeightArrayImp : public CDocWeightArrayImp
{
						CSimpleDocWeightArrayImp(HUrl& inFile, uint32 inCount);
						CSimpleDocWeightArrayImp(HFileStream& inFile,
							int64 inOffset, uint32 inCount);
						~CSimpleDocWeightArrayImp();

	virtual float		at(uint32 inIndex) const;
	
	HFileStream*		fFile;
	HMMappedFileStream*	fMap;
	const float*		fData;
	uint32				fCount;
};

CSimpleDocWeightArrayImp::CSimpleDocWeightArrayImp(HUrl& inFile, uint32 inCount)
	: CDocWeightArrayImp(inCount)
	, fFile(new HFileStream(inFile, O_RDONLY))
	, fMap(new HMMappedFileStream(*fFile, 0, inCount * sizeof(float)))
	, fData(static_cast<const float*>(fMap->Buffer()))
{
}

CSimpleDocWeightArrayImp::CSimpleDocWeightArrayImp(HFileStream& inFile, int64 inOffset, uint32 inCount)
	: CDocWeightArrayImp(inCount)
	, fFile(nil)
	, fMap(new HMMappedFileStream(inFile, inOffset, inCount * sizeof(float)))
	, fData(static_cast<const float*>(fMap->Buffer()))
{
}

CSimpleDocWeightArrayImp::~CSimpleDocWeightArrayImp()
{
	delete fMap;
	delete fFile;
}

float CSimpleDocWeightArrayImp::at(uint32 inIndex) const
{
	return net_swapper::swap(fData[inIndex]);
}

struct CJoinedDocWeightArrayImp : public CDocWeightArrayImp
{
						CJoinedDocWeightArrayImp(
							CDocWeightArrayImp* inFirst,
							CDocWeightArrayImp* inSecond);
						~CJoinedDocWeightArrayImp();

	virtual float		at(uint32 inIndex) const;
	
	CDocWeightArrayImp*	fFirst;
	CDocWeightArrayImp*	fSecond;
};

CJoinedDocWeightArrayImp::CJoinedDocWeightArrayImp(
		CDocWeightArrayImp* inFirst, CDocWeightArrayImp* inSecond)
	: CDocWeightArrayImp(inFirst->fCount + inSecond->fCount)
	, fFirst(inFirst)
	, fSecond(inSecond)
{
	fFirst->Reference();
	fSecond->Reference();
}

CJoinedDocWeightArrayImp::~CJoinedDocWeightArrayImp()
{
	fFirst->Release();
	fSecond->Release();
}

float CJoinedDocWeightArrayImp::at(uint32 inIndex) const
{
	float result;
	
	if (inIndex < fFirst->fCount)
		result = fFirst->at(inIndex);
	else
		result = fSecond->at(inIndex - fFirst->fCount);
	
	return result;
}

CDocWeightArray::CDocWeightArray(HUrl& inFile, uint32 inCount)
	: fImpl(new CSimpleDocWeightArrayImp(inFile, inCount))
{
}

CDocWeightArray::CDocWeightArray(HFileStream& inFile, int64 inOffset, uint32 inCount)
	: fImpl(new CSimpleDocWeightArrayImp(inFile, inOffset, inCount))
{
}

CDocWeightArray::CDocWeightArray(const CDocWeightArray& inOtherA,
		const CDocWeightArray& inOtherB)
	: fImpl(new CJoinedDocWeightArrayImp(inOtherA.fImpl, inOtherB.fImpl))
{
}

CDocWeightArray::CDocWeightArray(const CDocWeightArray& inOther)
	: fImpl(inOther.fImpl)
{
	fImpl->Reference();
}

CDocWeightArray& CDocWeightArray::operator=(const CDocWeightArray& inOther)
{
	if (this != &inOther)
	{
		fImpl->Release();
		fImpl = inOther.fImpl;
		fImpl->Reference();
	}
	
	return *this;
}

CDocWeightArray::~CDocWeightArray()
{
	fImpl->Release();
}

float CDocWeightArray::operator[](uint32 inDocNr) const
{
	if (inDocNr >= fImpl->fCount)
		THROW(("Index for doc weights (%d) is out of range", inDocNr));
	return fImpl->at(inDocNr);
}

