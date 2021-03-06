/*	$Id: CDatabank.h,v 1.68 2005/10/11 13:17:31 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:41:32
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
 
#ifndef CDATABANK_H
#define CDATABANK_H

#include "HStlVector.h"
#include "HStlString.h"
#include "HStdCString.h"

#include "HUrl.h"

#include "CBitStream.h"
#include "CIterator.h"
#include "CSequence.h"

class HStreamBase;
class CDecompressor;
class CCompressor;
class CIndexer;
class CDbInfo;
class CDatabankBase;
class CIdTable;
class CBlastIndex;
class CDictionary;

struct SIndexPart;
struct SHeader;
struct SDataHeader;
struct SDataPart;
struct SBlastIndexHeader;

struct ProgressInfo
{
	int64			parsed;
	float			perc;
	uint32			recs;
};

class CDatabankBase
{
  public:
						CDatabankBase();
	virtual				~CDatabankBase();
	
	virtual std::string	GetDocument(uint32 inDocNr) = 0;
	std::string			GetDocument(const std::string& inDocID);

	virtual std::string	GetDocumentID(uint32 inDocNr) const;
	virtual uint32		GetDocumentNr(const std::string& inDocID, bool inThrowIfNotFound = true) const;

	virtual uint32		GetBlastDbCount() const = 0;
	virtual int64		GetBlastDbLength() const = 0;
	virtual uint32		CountSequencesForDocument(uint32 inDocNr) = 0;

	std::string			GetSequence(const std::string& inDocID, uint32 inIndex);
	std::string			GetSequence(uint32 inDocNr, uint32 inIndex);

	virtual void		GetSequence(uint32 inDocNr, uint32 inIndex,
							CSequence& outSequence) = 0;
	
	virtual uint32		Count() const = 0;
	virtual std::string	GetVersion() const = 0;
	virtual void		PrintInfo() = 0;
//	virtual void		DumpIndex(const std::string& inIndex);
	virtual CIteratorBase*
						GetIteratorForIndex(const std::string& inIndex);
	virtual CIteratorBase*
						GetIteratorForIndexAndKey(const std::string& inIndex,
							const std::string& inKey);

	virtual long		GetIndexCount() = 0;
	virtual	void		GetIndexInfo(uint32 inIndexNr, std::string& outCode,
							std::string& outType, uint32& outCount) = 0;
							
						// next function returns IndexCount if not found... beware
	uint32				GetIndexNr(const std::string& inIndexName);
	
	virtual uint32		CountDocumentsContainingKey(const std::string& inIndex,
							const std::string& inKey) = 0;
	
	virtual std::vector<std::string>
						SuggestCorrection(const std::string& inKey);
	
	virtual std::string	GetDbName() const;
	virtual std::string	GetDbNameForDocID(const std::string& inDocID) const;
	
	virtual CDocIterator* CreateDocIterator(const std::string& inIndex,
							const std::string& inKey, bool inKeyIsPattern,
							CQueryOperator inOperator) = 0;
};

class CDatabank : public CDatabankBase
{
  public:
							// third constructor, for perl plugin interface
						CDatabank(const HUrl& inFile, bool inNew);
					
						~CDatabank();

	void				Merge(std::vector<CDatabank*>& inParts);
	virtual void		DumpIndex(const std::string& inIndex);
	virtual CIteratorBase*
						GetIteratorForIndex(const std::string& inIndex);
	virtual CIteratorBase*
						GetIteratorForIndexAndKey(const std::string& inIndex,
							const std::string& inKey);
	virtual void		PrintInfo();

	virtual std::string	GetDocument(uint32 inDocNr);

	virtual std::string	GetDocumentID(uint32 inDocNr) const;
	virtual uint32		GetDocumentNr(const std::string& inDocID, bool inThrowIfNotFound = true) const;
	
	virtual uint32		GetBlastDbCount() const;
	virtual int64		GetBlastDbLength() const;
	virtual uint32		CountSequencesForDocument(uint32 inDocNr);
	virtual void		GetSequence(uint32 inDocNr, uint32 inIndex,
							CSequence& outSequence);
	
	// for the perl interface
	void				Store(const std::string& inDocument);
	void				IndexText(const std::string& inIndex, const std::string& inText);
	void				IndexTextAndNumbers(const std::string& inIndex, const std::string& inText);
	void				IndexWord(const std::string& inIndex, const std::string& inText);
	void				IndexValue(const std::string& inIndex, const std::string& inText);
	void				IndexDate(const std::string& inIndex, const std::string& inText);
	void				IndexNumber(const std::string& inIndex, const std::string& inText);
	void				AddSequence(const std::string& inSequence);
	void				FlushDocument();

	void				SetVersion(const std::string& inVersion);

	void				Finish();
	
	virtual uint32		Count() const;
	virtual std::string	GetVersion() const;
	
	virtual long		GetIndexCount();
	virtual	void		GetIndexInfo(uint32 inIndexNr, std::string& outCode,
							std::string& outType, uint32& outCount);

	virtual uint32		CountDocumentsContainingKey(const std::string& inIndex,
							const std::string& inKey);

	void				CreateDictionaryForIndexes(
							const std::vector<std::string>& inIndexNames,
							uint32 inMinOccurrence, uint32 inMinWordLength);

	virtual std::vector<std::string>
						SuggestCorrection(const std::string& inKey);

	virtual CDocIterator* CreateDocIterator(const std::string& inIndex,
						const std::string& inKey, bool inKeyIsPattern,
						CQueryOperator inOperator);
	
	CDocIterator*
					GetImpForPattern(const std::string& inIndex,
						const std::string& inKey);
	
	HStreamBase&	GetDataFile()			{ return *fDataFile; }
	HUrl			GetDataUrl()			{ return fPath; }
	CIndexer*		GetIndexer();
	CBlastIndex*	GetBlastIndex() const	{ return fBlastIndex; }

	virtual std::string	GetDbName() const;
	
  private:

					CDatabank(const CDatabank&);
	CDatabank&		operator=(const CDatabank&);

	CDecompressor*	GetDecompressor(uint32 inPart);

	struct CDataPart
	{
		CDecompressor*	fDecompressor;
		uint32			fCount;
	};
	typedef std::vector<CDataPart> CPartList;
	
	HUrl			fPath;
	HStreamBase*	fDataFile;
	CCompressor*	fCompressor;
	CPartList		fDataParts;
	mutable CIndexer*
					fIndexer;
	bool			fReadOnly;
	std::string		fScript;
	CDbInfo*		fInfoContainer;
	CIdTable*		fIdTable;
	CBlastIndex*	fBlastIndex;
	CDictionary*	fDictionary;

	// on disk info
	SHeader*		fHeader;
	SDataHeader*	fDataHeader;
	SDataPart*		fParts;
	SBlastIndexHeader*	fBlast;
};

class CJoinedDatabank : public CDatabankBase
{
  public:
						CJoinedDatabank(std::vector<CDatabankBase*>& inParts);
						~CJoinedDatabank();
	
	virtual std::string	GetDocument(uint32 inDocNr);

	virtual std::string	GetDocumentID(uint32 inDocNr) const;
	virtual uint32		GetDocumentNr(const std::string& inDocID, bool inThrowIfNotFound = true) const;

	virtual uint32		GetBlastDbCount() const;
	virtual int64		GetBlastDbLength() const;
	virtual uint32		CountSequencesForDocument(uint32 inDocNr);
	virtual void		GetSequence(uint32 inDocNr, uint32 inIndex,
							CSequence& outSequence);

	virtual uint32		Count() const;
	virtual std::string	GetVersion() const;
	virtual void		PrintInfo();
	virtual long		GetIndexCount();
	virtual	void		GetIndexInfo(uint32 inIndexNr, std::string& outCode,
							std::string& outType, uint32& outCount);
	
	virtual uint32		CountDocumentsContainingKey(const std::string& inIndex,
							const std::string& inKey);

	virtual CDocIterator* CreateDocIterator(const std::string& inIndex,
							const std::string& inKey, bool inKeyIsPattern,
							CQueryOperator inOperator);

  private:
	
		// return part and docNr in part for docNr
	bool				PartForDoc(uint32& ioDocNr, CDatabankBase*& outDb) const;

	struct CPartInfo
	{
		CDatabankBase*	fDb;
		uint32			fCount;
	};
	
	struct CJoinedIndexInfo
	{
		std::string		fCode;
		std::string		fType;
		uint32			fCount;
	};

	CPartInfo*			fParts;
	uint32				fPartCount;
	CJoinedIndexInfo*	fIndices;
	uint32				fIndexCount;
	uint32				fCount;
};

class CUpdatedDatabank : public CDatabank
{
  public:
						CUpdatedDatabank(const HUrl& inFile, CDatabankBase* inOriginal);
						~CUpdatedDatabank();

	virtual uint32		Count() const;
	virtual std::string	GetVersion() const;
	virtual void		PrintInfo();

	virtual std::string	GetDocument(uint32 inDocNr);

	virtual std::string	GetDocumentID(uint32 inDocNr) const;
	virtual uint32		GetDocumentNr(const std::string& inDocID, bool inThrowIfNotFound = true) const;

	virtual uint32		GetBlastDbCount() const;
	virtual int64		GetBlastDbLength() const;
	virtual uint32		CountSequencesForDocument(uint32 inDocNr);
	virtual void		GetSequence(uint32 inDocNr, uint32 inIndex,
							CSequence& outSequence);

	virtual uint32		CountDocumentsContainingKey(const std::string& inIndex,
							const std::string& inKey);

	virtual std::vector<std::string>
						SuggestCorrection(const std::string& inKey);

	virtual CDocIterator* CreateDocIterator(const std::string& inIndex,
							const std::string& inKey, bool inKeyIsPattern,
							CQueryOperator inOperator);

  private:
	CDatabankBase*		fOriginal;
};

#endif // CDATABANK_H
