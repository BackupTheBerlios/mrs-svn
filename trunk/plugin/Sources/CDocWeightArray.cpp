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
	~CDocWeightArrayImp()
	{
		delete fMap;
		delete fFile;
	}
	
	HFileStream*		fFile;
	HMMappedFileStream*	fMap;
	const float*		fData;
	uint32				fCount;
	uint32				fRefCount;
};

CDocWeightArray::CDocWeightArray(HUrl& inFile, uint32 inCount)
	: fImpl(new CDocWeightArrayImp)
{
	fImpl->fFile = new HFileStream(inFile, O_RDONLY);
	fImpl->fMap = new HMMappedFileStream(*fImpl->fFile, 0, inCount * sizeof(float));
	fImpl->fData = static_cast<const float*>(fImpl->fMap->Buffer());
	fImpl->fCount = inCount;
	fImpl->fRefCount = 1;
}

CDocWeightArray::CDocWeightArray(HFileStream& inFile, int64 inOffset, uint32 inCount)
	: fImpl(new CDocWeightArrayImp)
{
	fImpl->fFile = nil;
	fImpl->fMap = new HMMappedFileStream(inFile, inOffset, inCount * sizeof(float));
	fImpl->fData = static_cast<const float*>(fImpl->fMap->Buffer());
	fImpl->fCount = inCount;
	fImpl->fRefCount = 1;
}

CDocWeightArray::CDocWeightArray(const CDocWeightArray& inOther)
	: fImpl(inOther.fImpl)
{
	++fImpl->fRefCount;
}

CDocWeightArray& CDocWeightArray::operator=(const CDocWeightArray& inOther)
{
	if (this != &inOther)
	{
		if (--fImpl->fRefCount == 0)
			delete fImpl;

		fImpl = inOther.fImpl;

		++fImpl->fRefCount;
	}
	
	return *this;
}

CDocWeightArray::~CDocWeightArray()
{
	if (--fImpl->fRefCount == 0)
		delete fImpl;
}

float CDocWeightArray::operator[](uint32 inDocNr) const
{
	if (inDocNr >= fImpl->fCount)
		THROW(("Index for doc weights (%d) is out of range", inDocNr));
	return net_swapper::swap(fImpl->fData[inDocNr]);
}

