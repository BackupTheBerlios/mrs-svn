/*	$Id: CDecompress.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Sunday December 08 2002 11:34:40
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
 
#ifndef CDECOMPRESS_H
#define CDECOMPRESS_H

#include <string>

#include "CDatabank.h"

class CSContext;

template<class T>
class CCArray;

class CDecompressor
{
  public:
						CDecompressor(const HUrl& inDb,
							HStreamBase& inFile, uint32 inKind,
							int64 inDataOffset, int64 inDataSize,
							int64 inTableOffset, int64 inTableSize,
							uint32 inMetaDataCount);
						~CDecompressor();
	
	std::string			GetDocument(uint32 inEntry);
	std::string			GetField(uint32 inEntry, uint32 inIndex);
	
	void				CopyData(HStreamBase& outData, uint32& outKind,
							int64& outDataOffset, int64& outDataSize,
							int64& outTableOffset, int64& outTableSize);

	void				LinkData(const std::string& inDataFileName,
							const std::string& inDataFileUUID,
							HStreamBase& outData, uint32& outKind,
							int64& outDataOffset, int64& outDataSize,
							int64& outTableOffset, int64& outTableSize);
	
  private:

	struct CDecompressorImp*	fImpl;
	uint32						fKind;
	uint32						fMetaDataCount;
};

#endif // CDECOMPRESS_H
