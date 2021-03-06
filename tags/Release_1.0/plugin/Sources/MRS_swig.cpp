/*
 * $Id: MRS_swig.cpp,v 1.49 2005/10/11 13:17:31 maarten Exp $
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

#include "MRS_swig.h"

#include "HStlIOStream.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <bzlib.h>

#include "HStdCStdlib.h"

#include "HError.h"
#include "HStream.h"
#include "HFile.h"
#include "HFileCache.h"
#include "HUtils.h"

#include "CDatabank.h"
#include "CQuery.h"
#include "CBlast.h"

#include "CIndexPage.h"		// for CIndexIterator

using namespace std;

#pragma export on

int			VERBOSE = 0;
int			THREADS = 1;
const char*	COMPRESSION = "zlib";
int			COMPRESSION_LEVEL = 6;
const char*	COMPRESSION_DICTIONARY = "";
string		gErrStr;

string errstr()
{
	return gErrStr;
}

template<class Derived, class Impl>
MRSObject<Derived,Impl>::~MRSObject()
{
	delete fImpl;
}

// ---------------------------------------------------------------------------
//
//  struct MDatabankImp
//

struct MDatabankImp
{
					MDatabankImp(const string& inDatabank, bool inNew);

	CDatabank*		GetDB()
					{
						CDatabank* result = dynamic_cast<CDatabank*>(fDatabank.get());
						if (result == nil) THROW(("Not a valid databank for this operation"));
						return result;
					}

	auto_ptr<CDatabankBase>		fDatabank;
	auto_ptr<HFile::SafeSaver>	fSafe;
	string						fScratch;		// used to return const char* 
};

// ---------------------------------------------------------------------------
//
//  struct MQueryResultsImp
//

struct MQueryResultsImp
{
								MQueryResultsImp(CDatabankBase& inDb,
										const string& inQuery, bool inAutoWildcard)
									: fDatabank(&inDb)
									, fQuery(inQuery, inDb, inAutoWildcard) {}
	
	CDatabankBase*				fDatabank;
	CQuery						fQuery;
	string						fScratch;
};

// ---------------------------------------------------------------------------
//
//  struct MKeysImp
//

struct MKeysImp
{
	auto_ptr<CIteratorBase>		fIter;
	string						fScratch;
};

// ---------------------------------------------------------------------------
//
//  struct MIndexImp
//

struct MIndexImp
{
	string						fCode;
	string						fType;
	uint32						fCount;
	uint32						fNr;
	CDatabankBase*				fDatabank;
	
	MKeys*						Keys();
	MKeys*						FindKey(const string& inKey);
};

// ---------------------------------------------------------------------------
//
//  struct MIndicesImp
//

struct MIndicesImp
{
	uint32						fCount;
	uint32						fCurrent;
	CDatabankBase*				fDatabank;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHitImp
//

struct MBlastHitImp
{
	string						fId;
	auto_ptr<CBlastHspIterator>	fHsps;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHitsImp
//

struct MBlastHitsImp
{
								MBlastHitsImp(const string& inQuery, const string& inMatrix,
										uint32 inWordSize, double inExpect,
										bool inFilter, bool inGapped,
										uint32 inGapOpen, uint32 inGapExtend)
									: fBlast(inQuery, inMatrix, inWordSize, inExpect,
										inFilter, inGapped, inGapOpen, inGapExtend)
									, fEOF(false) {}

	CBlast						fBlast;
	auto_ptr<CBlastHitIterator>	fHits;
	string						fScratch;
	bool						fEOF;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHspImp
//

struct MBlastHspImp
{
	uint32						fQueryStart;
	uint32						fSubjectStart;
	string						fQueryAlignment;
	string						fSubjectAlignment;
	uint32						fScore;
	double						fBitScore;
	double						fExpect;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHspsImp
//

struct MBlastHspsImp
{
	auto_ptr<CBlastHspIterator>	fHsps;
	bool						fEOF;
};

// ---------------------------------------------------------------------------
//
//  class MDatabank and struct MDatabankImp
//

MDatabankImp::MDatabankImp(const string& inDatabank, bool inNew)
{
	if (inNew)
	{
		HUrl url;
		url.SetNativePath(inDatabank);
		
		if (HFile::Exists(url))
		{
			fSafe.reset(new HFile::SafeSaver(url));
			url = fSafe->GetURL();
		}
		
		if (VERBOSE)
			cout << "Creating databank " << url.GetURL() << endl;
		
		fDatabank.reset(new CDatabank(url, true));
	}
	else
	{
		const char* data_dir = getenv("MRS_DATA_DIR");

		if (VERBOSE >= 2)
		{
			if (data_dir)
				cout << "using MRS_DATA_DIR=" << data_dir << endl;
			else
				cout << "MRS_DATA_DIR is not defined" << endl;
		}

		string databank(inDatabank);
		
		do
		{
			string::size_type p = databank.find('|');
			string db;
			
			if (p != string::npos)
			{
				db = databank.substr(0, p);
				databank.erase(0, p + 1);
			}
			else
			{
				db = databank;
				databank.clear();
			}
			
			HUrl url;
			
			string::size_type j = db.find('+');
			if (j != string::npos)
			{
				// joined parts are only allowed before the first updating part...
				
				if (fDatabank.get() != nil)
					THROW(("Joined parts can only occur before the first update part"));
				
				vector<CDatabankBase*> parts;
				
				do
				{
					string part;
					
					if (j != string::npos)
					{
						part = db.substr(0, j);
						db.erase(0, j + 1);
					}
					else
					{
						part = db;
						db.clear();
					}
					
					HUrl url;
		
					// find the db to use and try to be intelligent...
					
					url.SetNativePath(part);
					if (not HFile::Exists(url))
						url.SetNativePath(part + ".cmp");
					if (not HFile::Exists(url) and data_dir != nil)
						url.SetNativePath(string(data_dir) + '/' + part);
					if (not HFile::Exists(url) and data_dir != nil)
						url.SetNativePath(string(data_dir) + '/' + part + ".cmp");
			
					if (not HFile::Exists(url))
						THROW(("Databank not found: '%s'", part.c_str()));
	
					if (VERBOSE)
						cout << "Using file " << url.GetURL() << endl;
					
					parts.push_back(new CDatabank(url, false));
						
					j = db.find('+');
				}
				while (db.length());
				
				if (parts.size() == 0)
					THROW(("no databanks specified"));
	
				fDatabank.reset(new CJoinedDatabank(parts));
			}
			else
			{
	
				// find the db to use and try to be intelligent...
				
				url.SetNativePath(db);
				if (not HFile::Exists(url))
					url.SetNativePath(db + ".cmp");
				if (not HFile::Exists(url) and data_dir != nil)
					url.SetNativePath(string(data_dir) + '/' + db);
				if (not HFile::Exists(url) and data_dir != nil)
					url.SetNativePath(string(data_dir) + '/' + db + ".cmp");
		
				if (not HFile::Exists(url))
					THROW(("Databank not found: '%s'", db.c_str()));
				
				if (VERBOSE)
					cout << "Using file " << url.GetURL() << endl;
				
				if (fDatabank.get() == nil)
					fDatabank.reset(new CDatabank(url, false));
				else
					fDatabank.reset(new CUpdatedDatabank(url, fDatabank.release()));
			}
		}
		while (databank.length() != 0);
	}
}

MDatabank::MDatabank(const string& inDatabank)
	: MRSObject<MDatabank, struct MDatabankImp>(new MDatabankImp(inDatabank, false))
{
}

MDatabank::MDatabank(const string& inDatabank, bool)
	: MRSObject<MDatabank, struct MDatabankImp>(new MDatabankImp(inDatabank, true))
{
}

MDatabank* MDatabank::Create(const string& inName)
{
	return new MDatabank(inName, true);
}

void MDatabank::Merge(const string& inName, MDatabankArray inDbs)
{
	auto_ptr<MDatabankImp> result(new MDatabankImp(inName, true));
	
	vector<CDatabank*> dbs;
	for (vector<MDatabank*>::const_iterator db = inDbs.begin(); db != inDbs.end(); ++db)
	{
		MDatabank* d = *db;
		dbs.push_back(d->fImpl->GetDB());
	}
	
	dynamic_cast<CDatabank*>(result->GetDB())->Merge(dbs);

	if (result->fSafe.get())
		result->fSafe->Commit();
}

long MDatabank::Count()
{
	return fImpl->fDatabank->Count();
}

string MDatabank::GetVersion()
{
	return fImpl->fDatabank->GetVersion();
}

void MDatabank::DumpInfo()
{
	fImpl->fDatabank->PrintInfo();
}

long MDatabank::CountForKey(const string& inIndex, const string& inKey) const
{
	return fImpl->fDatabank->CountDocumentsContainingKey(inIndex, inKey);
}

MQueryResults* MDatabank::Find(const string& inQuery, bool inAutoWildcard)
{
	auto_ptr<MQueryResultsImp> imp(new MQueryResultsImp(*fImpl->fDatabank, inQuery, inAutoWildcard));
	MQueryResults* result = NULL;

	if (imp->fQuery.Count(false) > 0)
		result = MQueryResults::Create(imp.release());

	return result;
}

const char* MDatabank::Get(const string& inEntryID)
{
	const char* result = nil;
	
	try
	{
		fImpl->fScratch = fImpl->fDatabank->GetDocument(inEntryID);
		result = fImpl->fScratch.c_str();
	}
	catch (exception& e)
	{
		gErrStr = e.what();
	}
	
	return result;
}

const char* MDatabank::Sequence(const string& inEntryID, unsigned long inIndex)
{
	const char* result = nil;
	
	try
	{
		fImpl->fScratch = fImpl->fDatabank->GetSequence(inEntryID, inIndex);
		result = fImpl->fScratch.c_str();
	}
	catch (exception& e)
	{
		gErrStr = e.what();
	}
	
	return result;
}

MBlastHits* MDatabank::Blast(const string& inQuery, const std::string& inMatrix,
	unsigned long inWordSize, double inExpect, bool inFilter,
	bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend)
{
	MBlastHits* result = nil;
	
	CQuery query("*", *fImpl->fDatabank, false);
	auto_ptr<MBlastHitsImp> impl(new MBlastHitsImp(inQuery, inMatrix,
		inWordSize, inExpect, inFilter, inGapped, inGapOpen, inGapExtend));
	
	if (impl->fBlast.Find(query))
	{
		impl->fHits.reset(new CBlastHitIterator(impl->fBlast.Hits()));
		result = MBlastHits::Create(impl.release());
	}
			
	return result;
}

MIndex* MDatabank::Index(const std::string& inIndex)
{
	auto_ptr<MIndices> indices(Indices());
	MIndex* result = nil;
	
	for (;;)
	{
		auto_ptr<MIndex> index(indices->Next());
		if (index.get() == nil)
			break;
		
		if (index->Code() == inIndex)
		{
			result = index.release();
			break;
		}
	}
	
	return result;
}

MIndices* MDatabank::Indices()
{
	MIndices* result = nil;

	auto_ptr<MIndicesImp> impl(new MIndicesImp);
	impl->fCount = fImpl->fDatabank->GetIndexCount();
	impl->fCurrent = 0;
	impl->fDatabank = fImpl->fDatabank.get();
	
	if (impl->fCount > 0)
		result = MIndices::Create(impl.release());
	
	return result;
}

MStringIterator* MDatabank::SuggestCorrection(const string& inWord)
{
	MStringIterator* result = nil;

	vector<string> v = fImpl->fDatabank->SuggestCorrection(inWord);
	if (v.size())
		result = new MStringIterator(v);
	
	return result;
}

// interface for creating new databanks

void MDatabank::Store(const string& inDocument)
{
	fImpl->GetDB()->Store(inDocument);
}

void MDatabank::IndexText(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexText(inIndex, inText);
}

void MDatabank::IndexTextAndNumbers(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexTextAndNumbers(inIndex, inText);
}

void MDatabank::IndexWord(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexWord(inIndex, inText);
}

void MDatabank::IndexValue(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexValue(inIndex, inText);
}

void MDatabank::IndexDate(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexDate(inIndex, inText);
}

void MDatabank::IndexNumber(const string& inIndex, const string& inText)
{
	fImpl->GetDB()->IndexNumber(inIndex, inText);
}

void MDatabank::AddSequence(const string& inSequence)
{
	fImpl->GetDB()->AddSequence(inSequence);
}

void MDatabank::FlushDocument()
{
	fImpl->GetDB()->FlushDocument();
}

void MDatabank::SetVersion(const string& inVersion)
{
	fImpl->GetDB()->SetVersion(inVersion);
}

void MDatabank::Finish()
{
	fImpl->GetDB()->Finish();
	
	HFileCache::Flush();
	
	if (fImpl->fSafe.get())
		fImpl->fSafe->Commit();
}

// stupid swig...
// now we have to pass the indices contatenated as a string, separated by colon
void MDatabank::CreateDictionary(std::string inIndices, long inMinOccurrence, long inMinWordLength)
{
	vector<string> indices;
	
	string::size_type c;
	while ((c = inIndices.find(':')) != string::npos)
	{
		indices.push_back(inIndices.substr(0, c));
		inIndices.erase(0, c + 1);
	}
	indices.push_back(inIndices);
	
	fImpl->GetDB()->CreateDictionaryForIndexes(indices, inMinOccurrence, inMinWordLength);
}

// ---------------------------------------------------------------------------
//
//  class MQueryResults
//

const char* MQueryResults::Next()
{
	const char* result = nil;
	uint32 docNr;
	
	if (fImpl->fQuery.Next(docNr))
	{
		fImpl->fScratch = fImpl->fDatabank->GetDocumentID(docNr);
		result = fImpl->fScratch.c_str();
	}

	return result;
}

void MQueryResults::Skip(long inCount)
{
	if (inCount < 0)
		THROW(("Cannot skip backwards in query results"));
	
	uint32 docNr;
	while (inCount-- > 0)
		fImpl->fQuery.Next(docNr);
}

unsigned long MQueryResults::Count(bool inExact) const
{
	return fImpl->fQuery.Count(inExact);
}

MBlastHits* MQueryResults::Blast(const string& inQuery, const std::string& inMatrix,
	unsigned long inWordSize, double inExpect, bool inFilter,
	bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend)
{
	MBlastHits* result = nil;
	
	auto_ptr<MBlastHitsImp> impl(new MBlastHitsImp(inQuery, inMatrix, inWordSize,
		inExpect, inFilter, inGapped, inGapOpen, inGapExtend));
	
	if (impl->fBlast.Find(fImpl->fQuery))
	{
		impl->fHits.reset(new CBlastHitIterator(impl->fBlast.Hits()));
		result = MBlastHits::Create(impl.release());
	}
			
	return result;
}


// ---------------------------------------------------------------------------
//
//  class MKeys
//

const char* MKeys::Next()
{
	const char* result = nil;
	uint32 v;

	if (fImpl->fIter->Next(fImpl->fScratch, v))
		result = fImpl->fScratch.c_str();
	
	return result;
}

void MKeys::Skip(long inCount)
{
	string s;
	uint32 v;

	if (inCount > 0)
	{
		while (inCount-- > 0)
			fImpl->fIter->Next(s, v);
	}
	else
	{
		while (inCount++ < 0)
			fImpl->fIter->Prev(s, v);
	}
}

// ---------------------------------------------------------------------------
//
//  class MIndex
//

string MIndex::Code() const
{
	return fImpl->fCode;
}

string MIndex::Type() const
{
	return fImpl->fType;
}

long MIndex::Count() const
{
	return fImpl->fCount;
}

MKeys* MIndex::Keys()
{
	auto_ptr<MKeysImp> impl(new MKeysImp);
	
	impl->fIter.reset(fImpl->fDatabank->GetIteratorForIndex(Code()));
	
	return MKeys::Create(impl.release());
}

MKeys* MIndex::FindKey(const string& inKey)
{
	auto_ptr<MKeysImp> impl(new MKeysImp);
	
	impl->fIter.reset(fImpl->fDatabank->GetIteratorForIndexAndKey(Code(), inKey));
	
	return MKeys::Create(impl.release());
}

// ---------------------------------------------------------------------------
//
//  class MIndices
//

MIndex* MIndices::Next()
{
	MIndex* result = nil;

	if (fImpl->fCurrent < fImpl->fCount)
	{
		auto_ptr<MIndexImp> impl(new MIndexImp);
		impl->fDatabank = fImpl->fDatabank;
		impl->fNr = fImpl->fCurrent;
		
		fImpl->fDatabank->GetIndexInfo(impl->fNr, impl->fCode, impl->fType, impl->fCount);
		
		result = MIndex::Create(impl.release());
		
		++fImpl->fCurrent;
	}
	
	return result;
}


// ---------------------------------------------------------------------------
//
//  class MBlastHit
//

const char* MBlastHit::Id()
{
	return fImpl->fId.c_str();
}

MBlastHsps* MBlastHit::Hsps()
{
	auto_ptr<MBlastHspsImp> impl(new MBlastHspsImp);
	impl->fHsps.reset(new CBlastHspIterator(*fImpl->fHsps));
	impl->fEOF = false;
	return MBlastHsps::Create(impl.release());
}


// ---------------------------------------------------------------------------
//
//  class MBlastHits
//

const char* MBlastHits::ReportInXML()
{
	fImpl->fScratch = fImpl->fBlast.ReportInXML();
	return fImpl->fScratch.c_str();
}

MBlastHit* MBlastHits::Next()
{
	MBlastHit* result = nil;
	
	if (not fImpl->fEOF)
	{
		auto_ptr<MBlastHitImp> impl(new MBlastHitImp);
		impl->fId = fImpl->fHits->DocumentID();
		impl->fHsps.reset(new CBlastHspIterator(fImpl->fHits->Hsps()));
		result = MBlastHit::Create(impl.release());
		
		fImpl->fEOF = fImpl->fHits->Next();
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//
//  class MBlastHsp
//

unsigned long MBlastHsp::Score()
{
	return fImpl->fScore;
}

double MBlastHsp::BitScore()
{
	return fImpl->fBitScore;
}

double MBlastHsp::Expect()
{
	return fImpl->fExpect;
}

unsigned long MBlastHsp::QueryStart()
{
	return fImpl->fQueryStart;
}

unsigned long MBlastHsp::SubjectStart()
{
	return fImpl->fSubjectStart;
}

std::string MBlastHsp::QueryAlignment()
{
	return fImpl->fQueryAlignment;
}

std::string MBlastHsp::SubjectAlignment()
{
	return fImpl->fSubjectAlignment;
}

// ---------------------------------------------------------------------------
//
//  class MBlastHsps
//

MBlastHsp* MBlastHsps::Next()
{
	MBlastHsp* result = nil;
	
	if (not fImpl->fEOF)
	{
		auto_ptr<MBlastHspImp> impl(new MBlastHspImp);

		impl->fQueryStart = fImpl->fHsps->QueryStart();
		impl->fSubjectStart = fImpl->fHsps->SubjectStart();
		impl->fQueryAlignment = fImpl->fHsps->QueryAlignment();
		impl->fSubjectAlignment = fImpl->fHsps->SubjectAlignment();
		impl->fScore = fImpl->fHsps->Score();
		impl->fBitScore = fImpl->fHsps->BitScore();
		impl->fExpect = fImpl->fHsps->Expect();

		result = MBlastHsp::Create(impl.release());
		
		fImpl->fEOF = fImpl->fHsps->Next();
	}
	
	return result;
}

#pragma export off
