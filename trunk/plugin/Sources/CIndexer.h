/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 18:35:51
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
 
#ifndef CINDEXER_H
#define CINDEXER_H

#include "HStlSStream.h"
#include "HStlMap.h"

#include "CDatabank.h"
#include "CIterator.h"

class HStreamBase;
class CDecompressor;
class CIndexBase;
class CFullTextIndex;
class CIndexCache;
class CIndex;
class CIteratorBase;

class CDocWeightArray
{
  public:
					CDocWeightArray(HStreamBase& inFile, int64 inOffset, uint32 inCount);
	virtual			~CDocWeightArray();

	float			operator[](uint32 inDocNr) const;

  private:
	struct CDocWeightArrayImp*	fImpl;
};

class CIndexer
{
  public:
					CIndexer(const HUrl& inDb);
					CIndexer(HStreamBase& inFile,
						int64 inOffset, int64 inSize);
	virtual			~CIndexer();
	
	void			CreateIndex(HStreamBase& data,
						int64& outOffset, int64& outSize,
						bool inCreateAllTextIndex);
	
	void			MergeIndices(HStreamBase& outData,
						std::vector<CDatabank*>& inParts);
						
	uint32			Count() const;
	

	long			GetIndexCount() const;
	void			GetIndexInfo(uint32 inIndexNr, std::string& outCode,
						std::string& outType, uint32& outCount) const;

	CDocIterator*	GetImpForPattern(const std::string& inIndex,
						const std::string& inKey);
	CDocIterator*	CreateDocIterator(const std::string& inIndex,
						const std::string& inKey, bool inKeyIsPattern,
						CQueryOperator inOperator);
	// It would be better to return CIndexIterator * here, but that
	// gives trouble because that type needs inclusion of extra header
	// files.
	CIteratorBase*	GetIteratorForIndex(const std::string& inIndex);
	CIteratorBase*	GetIteratorForIndexAndKey(const std::string& inIndex, const std::string& inKey);
	CIndex*			GetIndex(const std::string& inIndex) const;
	CDbDocIteratorBase*
					GetDocWeightIterator(const std::string& inIndex, const std::string& inKey);
	
	// returns an array of floats
	const CDocWeightArray&
					GetDocWeights(const std::string& inIndex) const;

	void			PrintInfo();
	void			DumpIndex(const std::string& inIndex) const;
	
	void			IndexText(const std::string& inIndex, const std::string& inText);
	void			IndexTextAndNumbers(const std::string& inIndex, const std::string& inText);
	void			IndexWord(const std::string& inIndex, const std::string& inText);
	void			IndexDate(const std::string& inIndex, const std::string& inText);
	void			IndexNumber(const std::string& inIndex, const std::string& inText);
	void			IndexValue(const std::string& inIndex, const std::string& inText);
	void			IndexWordWithWeight(const std::string& inIndex,	
						const std::string& inText, uint32 inFrequency);

	uint32			GetDocumentNr(const std::string& inDocumentID,
						bool inThrowIfNotFound = true);

	void			FlushDoc();

	void			FixupDocWeights();
	void			RecalculateDocumentWeights(const std::string& inIndex);

  private:

	void			IndexText(const std::string& inIndex, const std::string& inText, bool inIndexNrs);
	
	CFullTextIndex&	GetFullTextIndex();
	
	std::string						fDb;
	std::stringstream				doc_text;
	std::map<std::string,CIndexBase*>indexes;
	CFullTextIndex*					fFullTextIndex;
	uint16							fNextTextIndexID;

	// for existing indices
	HStreamBase*					fFile;
	struct SIndexHeader*			fHeader;
	struct SIndexPart*				fParts;
	
	CDocWeightArray**				fDocWeights;
	
	// for statistics
	int64							fOffset;
	int64							fSize;
};

#endif // CINDEXER_H
