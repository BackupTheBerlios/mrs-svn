/*	$Id: CCompress.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Monday December 09 2002 14:15:21
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
 
#ifndef CCOMPRESS_H
#define CCOMPRESS_H

#include "HUrl.h"

#include <map>
#include <vector>

class HStreamBase;
class CTextTable;
class ProgressCallBackHandlerBase;
class CLexicon;
struct DatabankHeader;

enum CCompressionKind {
	kZLibCompressed = FOUR_CHAR_INLINE('mrsz'),
	kHuffWordCompressed = FOUR_CHAR_INLINE('mrsh'),
	kbzLibCompressed = FOUR_CHAR_INLINE('mrsb'),
	kInvalidData = FOUR_CHAR_INLINE('zero'),
	kMergedData = FOUR_CHAR_INLINE('mrsm'),
	kLinkedData = FOUR_CHAR_INLINE('link'),
	kUnCompressed = FOUR_CHAR_INLINE('none')
};

class CCompressor;
struct CCompressorImp;

class CCompressorFactory
{
  public:
						CCompressorFactory(
							const char*		inCompressionAlgorithmName,
							int32			inCompressionLevel,
							const char*		inDictionary,
							uint32			inDictionaryLength);

	void				InitDataStream(
							HStreamBase&	outData);

	CCompressor*		CreateCompressor();

	CCompressionKind	GetCompressionKind() const			{ return fKind; }

  private:
	CCompressionKind	fKind;
	int32				fLevel;
	std::string			fDictionary;
};

class CCompressor
{
  public:

	virtual				~CCompressor();

	void				CompressDocument(
							const char*		inText,
							uint32			inSize,
							HStreamBase&	outData);

	void				CompressData(
							std::vector<std::pair<const char*,uint32> >&
											inDataVector,
							HStreamBase&	outData);

  private:

	friend class CCompressorFactory;
	
						CCompressor(
							CCompressorImp*	inImpl);
						
						CCompressor(
							const CCompressor&);
	CCompressor&		operator=(
							const CCompressor&);

	CCompressorImp*		fImpl;
};

#endif // CCOMPRESS_H
