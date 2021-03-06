/*
 * $Id: MRSInterface.cpp 324 2007-02-08 12:36:05Z hekkel $
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

#include "MObjectInt.h"
#include "MRSInterface.h"

#include <boost/shared_ptr.hpp>
#include <boost/regex.hpp>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <bzlib.h>

#include <sys/times.h>
#include <sys/resource.h>

#include <cstdlib>

#include "HError.h"
#include "HStream.h"
#include "HFile.h"
#include "HUtils.h"

#include "CDatabank.h"
#include "CQuery.h"
#ifndef NO_BLAST
#include "CBlast.h"
#endif
#include "CRankedQuery.h"
#include "CIterator.h"
#include "CDocWeightArray.h"

#include "CIndex.h"		// for CIndexIterator

const char kAppName[] = "MRS";

using namespace std;

#pragma export on

int				VERBOSE = 0;
unsigned int	THREADS = 1;
const char*		COMPRESSION = "zlib";
int				COMPRESSION_LEVEL = 6;
const char*		COMPRESSION_DICTIONARY = "";
string			gErrStr;
const char*		kFileExtension = ".cmp";

string errstr()
{
	return gErrStr;
}

ostream& operator<<(ostream& inStream, const struct timeval& t)
{
	uint32 hours = t.tv_sec / 3600;
	uint32 minutes = ((t.tv_sec % 3600) / 60);
	uint32 seconds = t.tv_sec % 60;
	
	uint32 milliseconds = t.tv_usec / 1000;
	
	inStream << hours << ':'
			 << setw(2) << setfill('0') << minutes << ':'
			 << setw(2) << setfill('0') << seconds << '.'
			 << setw(3) << setfill('0') << milliseconds;
	
	return inStream;
}

class CStopwatch
{
  public:
			CStopwatch(double& ioAccumulator);
			~CStopwatch();

  private:
	double&			fAccumulator;
	struct rusage	fStartTime;
};

CStopwatch::CStopwatch(double& ioAccumulator)
	: fAccumulator(ioAccumulator)
{
	if (VERBOSE > 1)
		getrusage(RUSAGE_SELF, &fStartTime);
}

CStopwatch::~CStopwatch()
{
	if (VERBOSE > 1)
	{
		struct rusage stop;
		getrusage(RUSAGE_SELF, &stop);
		
		fAccumulator += (stop.ru_utime.tv_sec - fStartTime.ru_utime.tv_sec);
		fAccumulator += 0.000001 * (stop.ru_utime.tv_usec - fStartTime.ru_utime.tv_usec);
	}
}

static void PrintStatistics()
{
	struct rusage ru = {} ;
	
	if (getrusage(RUSAGE_SELF, &ru) < 0)
		cerr << "Error calling getrusage" << endl;
	
	cout << "Total time user:    " << ru.ru_utime << endl;
	cout << "Total time system:  " << ru.ru_stime << endl;
	cout << "I/O operations in:  " << ru.ru_inblock << endl;
	cout << "I/O operations out: " << ru.ru_oublock << endl;
}	
	
// ---------------------------------------------------------------------------
//
//  struct MDatabankImp
//

struct MDatabankImp
{
					MDatabankImp(const string& inDatabank);
					MDatabankImp(const string& inDatabank, const vector<string>& inMetaDataFields,
						const string& inName, const string& inVersion, const string& inURL,
						const string& inScriptName, const string& inSection);

	CDatabank*		GetDB()
					{
						CDatabank* result = dynamic_cast<CDatabank*>(fDatabank.get());
						if (result == nil) THROW(("Not a valid databank for this operation"));
						return result;
					}
	
	void			Close()
					{
						fDatabank.reset(nil);
					}

	void			PrintCreateStatistics();

	auto_ptr<CDatabankBase>		fDatabank;
	auto_ptr<HFile::SafeSaver>	fSafe;
	string						fScratch;		// used to return const char* 

	double						fStoreTime;		// statitistics
	double						fFlushTime;
	double						fIndexTime;
	double						fFinishTime;
};

// ---------------------------------------------------------------------------
//
//  struct MQueryResultsImp
//

struct MQueryResultsImp
{
								MQueryResultsImp(CDatabankBase& inDb,
										CDocIterator* inDocIterator,
										uint32 inCount = numeric_limits<uint32>::max())
									: fDatabank(&inDb)
									, fScore(1.0f)
									, fCount(inCount)
									, fRead(0)
									, fIter(inDocIterator) {}
	virtual						~MQueryResultsImp() {}

	virtual bool				Next(uint32& outDoc)
								{
									bool result = false;

									CDocFreqVectorIterator* vi = dynamic_cast<CDocFreqVectorIterator*>(fIter.get());
									if (vi != nil)
										result = vi->Next(outDoc, fScore, false);
									else if (fIter.get() != nil)
										result = fIter->Next(outDoc, false);
									
									if (result)
										++fRead;

									return result;
								}

	virtual uint32				Count(bool inExact)
								{
									uint32 result;

									if (fCount == numeric_limits<uint32>::max())
									{
										if (inExact)
										{
											fCount = fRead;
											
											uint32 d;
											while (fIter->Next(d, false))
												++fCount;
											
											result = fCount;
										}
										else if (fIter.get() != nil)
											result = fIter->Count();
									}
									else
										result = fCount;
									
									return result;
								}

	CDatabankBase*				fDatabank;
	string						fScratch;
	float						fScore;
	uint32						fCount;
	uint32						fRead;

	auto_ptr<CDocIterator>		fIter;
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

#ifndef NO_BLAST
// ---------------------------------------------------------------------------
//
//  struct MBlastHitImp
//

struct MBlastHitImp
{
	string						fId;
	string						fTitle;
	auto_ptr<CBlastHspIterator>	fHsps;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHitsImp
//

struct MBlastHitsImp
{
								MBlastHitsImp(CDatabankBase& inDb,
										const string& inQuery, const string& inMatrix,
										uint32 inWordSize, double inExpect,
										bool inFilter, bool inGapped,
										uint32 inGapOpen, uint32 inGapExtend)
									: fDb(inDb)
									, fBlast(inQuery, inMatrix, inWordSize, inExpect,
										inFilter, inGapped, inGapOpen, inGapExtend) {}

	CDatabankBase&				fDb;
	CBlast						fBlast;
	auto_ptr<CBlastHitIterator>	fHits;
	string						fScratch;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHspImp
//

struct MBlastHspImp
{
	uint32						fQueryStart;
	uint32						fSubjectStart;
	uint32						fSubjectLength;
	string						fQueryAlignment;
	string						fSubjectAlignment;
	string						fMidline;
	uint32						fScore;
	double						fBitScore;
	double						fExpect;
	uint32						fIdentity;
	uint32						fPositive;
	uint32						fGaps;
};

// ---------------------------------------------------------------------------
//
//  struct MBlastHspsImp
//

struct MBlastHspsImp
{
	auto_ptr<CBlastHspIterator>	fHsps;
};
#endif

// ---------------------------------------------------------------------------
//
//  struct MBooleanQueryImp
//

struct MBooleanQueryImp
{
	CDatabankBase*				fDatabank;
	boost::shared_ptr<CQueryObject>
								fQuery;
};

// ---------------------------------------------------------------------------
//
//  struct MRankedQueryImp
//

struct MRankedQueryImp
{
	CDatabankBase*				fDatabank;
	string						fIndex;
	string						fAlgorithm;
	bool						fAllTermsRequired;
	int							fMaxReturn;
	auto_ptr<CRankedQuery>		fQuery;
};

// ---------------------------------------------------------------------------
//
//  class MDatabank and struct MDatabankImp
//

const char MDatabank::kWildCardString[] = "*";

MDatabankImp::MDatabankImp(const string& inDatabank, const vector<string>& inMetaDataFields,
	const string& inName, const string& inVersion, const string& inURL, const string& inScriptName, const string& inSection)
	: fStoreTime(0)
	, fFlushTime(0)
	, fIndexTime(0)
	, fFinishTime(0)
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
	
	fDatabank.reset(new CDatabank(url, inMetaDataFields, inName, inVersion, inURL, inScriptName, inSection));
}

MDatabankImp::MDatabankImp(const string& inDatabank)
	: fStoreTime(0)
	, fFlushTime(0)
	, fIndexTime(0)
	, fFinishTime(0)
{
	const char* data_dir = getenv("MRS_DATA_DIR");

	if (VERBOSE >= 2)
	{
		if (data_dir)
			cout << "using MRS_DATA_DIR=" << data_dir << endl;
		else
			cout << "MRS_DATA_DIR is not defined" << endl;
	}

	boost::regex re("\\+");
	boost::sregex_token_iterator i(inDatabank.begin(), inDatabank.end(), re, -1), j;

	vector<CDatabankBase*> parts;
	
	while (i != j)
	{
		string part = *i;
		++i;
		
		HUrl url;
	
		// find the db to use and try to be intelligent...
		
		url.SetNativePath(part);
		if (not HFile::Exists(url))
			url.SetNativePath(part + kFileExtension);
		if (not HFile::Exists(url) and data_dir != nil)
			url.SetNativePath(string(data_dir) + '/' + part);
		if (not HFile::Exists(url) and data_dir != nil)
			url.SetNativePath(string(data_dir) + '/' + part + kFileExtension);

		if (not HFile::Exists(url))
			THROW(("Databank not found: '%s'", part.c_str()));

		if (VERBOSE)
			cout << "Using file " << url.GetURL() << endl;
		
		parts.push_back(new CDatabank(url));
	}
	
	if (parts.size() > 1)
		fDatabank.reset(new CJoinedDatabank(parts));
	else if (parts.size() == 1)
		fDatabank.reset(parts.front());
	else
		THROW(("Databank '%s' not found", inDatabank.c_str()));
}

void MDatabankImp::PrintCreateStatistics()
{
	cout << "Time spent on" << endl;

	struct timeval t;
	
	t.tv_sec = static_cast<uint32>(fStoreTime);
	t.tv_usec = static_cast<uint32>(fStoreTime * 1000000.0) % 1000000;
	
	cout << "Storing data:       " << t << endl;

	t.tv_sec = static_cast<uint32>(fFlushTime);
	t.tv_usec = static_cast<uint32>(fFlushTime * 1000000.0) % 1000000;
	
	cout << "Flushing data:      " << t << endl;

	t.tv_sec = static_cast<uint32>(fIndexTime);
	t.tv_usec = static_cast<uint32>(fIndexTime * 1000000.0) % 1000000;
	
	cout << "Indexing data:      " << t << endl;

	t.tv_sec = static_cast<uint32>(fFinishTime);
	t.tv_usec = static_cast<uint32>(fFinishTime * 1000000.0) % 1000000;
	
	cout << "Writing indices:    " << t << endl;
}

MDatabank::MDatabank()
{
	THROW(("Don't do this"));
}

MDatabank::MDatabank(const string& inDatabank)
	: MRSObject<MDatabank, struct MDatabankImp>(new MDatabankImp(inDatabank))
{
}

MDatabank::MDatabank(const string& inDatabank, const vector<string>& inMetaDataFields,
	const string& inName, const string& inVersion, const string& inURL, const string& inScriptName, const string& inSection)
	: MRSObject<MDatabank, struct MDatabankImp>(new MDatabankImp(inDatabank, inMetaDataFields,
		inName, inVersion, inURL, inScriptName, inSection))
{
}

MDatabank* MDatabank::Create(const string& inPath, vector<string> inMetaDataFields,
	const string& inName, const string& inVersion, const string& inURL, const string& inScriptName, const string& inSection)
{
	return new MDatabank(inPath, inMetaDataFields, inName, inVersion, inURL, inScriptName, inSection);
}

void MDatabank::Merge(const string& inPath, MDatabankArray inDbs, bool inCopyData,
	const string& inName, const string& inURL, const string& inScriptName, const string& inSection)
{
	if (inDbs.size() == 0 or inDbs.front() == nil)
		THROW(("Invalid array of databanks passed to Merge"));
	
	vector<string> metaData = inDbs[0]->fImpl->GetDB()->GetMetaDataFields();
	
	auto_ptr<MDatabankImp> result(new MDatabankImp(inPath, metaData, inName, "", inURL, inScriptName, inSection));
	
	vector<CDatabank*> dbs;
	for (vector<MDatabank*>::const_iterator db = inDbs.begin(); db != inDbs.end(); ++db)
	{
		MDatabank* d = *db;
		dbs.push_back(d->fImpl->GetDB());
	}
	
	dynamic_cast<CDatabank*>(result->GetDB())->Merge(dbs, inCopyData);

	if (result->fSafe.get())
		result->fSafe->Commit();

	// And now print out some statistics, if wanted of course
	if (VERBOSE > 1)
		PrintStatistics();
}

long MDatabank::Count()
{
	return fImpl->fDatabank->Count();
}

string MDatabank::GetCode()
{
	return fImpl->fDatabank->GetDbName();
}

string MDatabank::GetVersion()
{
	return fImpl->fDatabank->GetVersion();
}

string MDatabank::GetUUID()
{
	return fImpl->fDatabank->GetUUID();
}

string MDatabank::GetName()
{
	return fImpl->fDatabank->GetName();
}

string MDatabank::GetInfoURL()
{
	return fImpl->fDatabank->GetInfoURL();
}

string MDatabank::GetScriptName()
{
	return fImpl->fDatabank->GetScriptName();
}

string MDatabank::GetSection()
{
	return fImpl->fDatabank->GetSection();
}

bool MDatabank::IsUpToDate()
{
	return fImpl->fDatabank->IsUpToDate();
}

string MDatabank::GetFilePath()
{
	return fImpl->GetDB()->GetDataUrl().GetURL();
}

long long MDatabank::GetRawDataSize()
{
	return fImpl->fDatabank->GetRawDataSize();
}

void MDatabank::DumpInfo()
{
	fImpl->fDatabank->PrintInfo();
}

void MDatabank::DumpIndex(const string& inIndex)
{
	fImpl->GetDB()->DumpIndex(inIndex);
}

void MDatabank::PrefetchDocWeights(const string& inIndex)
{
	CDocWeightArray a(fImpl->fDatabank->GetDocWeights(inIndex));
	a.Prefetch();
}

long MDatabank::CountForKey(const string& inIndex, const string& inKey) const
{
	return fImpl->fDatabank->CountDocumentsContainingKey(inIndex, inKey);
}

MBooleanQuery* MDatabank::Match(const string& inValue, const string& inIndex)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);
	imp->fDatabank = fImpl->fDatabank.get();
	imp->fQuery.reset(new CMatchQueryObject(*imp->fDatabank, inValue, inIndex));
	return MBooleanQuery::Create(imp.release());
}

MBooleanQuery* MDatabank::MatchRel(const string& inValue, const string& inRelOp, const string& inIndex)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);
	imp->fDatabank = fImpl->fDatabank.get();
	imp->fQuery.reset(new CMatchQueryObject(*imp->fDatabank, inValue, inRelOp, inIndex));
	return MBooleanQuery::Create(imp.release());
}

MQueryResults* MDatabank::Find(const string& inQuery, bool inAutoWildcard)
{
	auto_ptr<CParsedQueryObject> parser(
		new CParsedQueryObject(*fImpl->fDatabank, inQuery, inAutoWildcard));
	
	auto_ptr<MQueryResultsImp> imp(new MQueryResultsImp(*fImpl->fDatabank, parser->Perform()));
	MQueryResults* result = NULL;

	if (imp->Count(false) > 0)
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

bool MDatabank::Get(const string& inEntryID, string& outText)
{
	bool result = false;
	
	try
	{
		outText = fImpl->fDatabank->GetDocument(inEntryID);
		result = true;
	}
	catch (exception& e)
	{
		outText.clear();
		gErrStr = e.what();
	}
	
	return result;
}

const char* MDatabank::GetMetaData(const string& inEntryID, const string& inFieldName)
{
	const char* result = nil;
	
	try
	{
		fImpl->fScratch = fImpl->fDatabank->GetMetaData(inEntryID, inFieldName);
		result = fImpl->fScratch.c_str();
	}
	catch (exception& e)
	{
		gErrStr = e.what();
	}
	
	return result;
}

bool MDatabank::GetMetaData(const string& inEntryID, const string& inFieldName,
		string& outText)
{
	bool result = false;
	
	try
	{
		outText = fImpl->fDatabank->GetMetaData(inEntryID, inFieldName);
		result = true;
	}
	catch (exception& e)
	{
		outText.clear();
		gErrStr = e.what();
	}
	
	return result;
}

const char* MDatabank::GetDescription(const string& inEntryID)
{
	const char* result = nil;
	
	try
	{
		MStringArray mf(fImpl->fDatabank->GetMetaDataFields());
		
		if (mf.size() > 0)
		{
			fImpl->fScratch = fImpl->fDatabank->GetMetaData(inEntryID, mf[0].c_str());
			result = fImpl->fScratch.c_str();
		}
	}
	catch (exception& e)
	{
		gErrStr = e.what();
	}
	
	return result;
}

bool MDatabank::GetDescription(const string& inEntryID, string& outText)
{
	bool result = false;
	
	try
	{
		MStringArray mf(fImpl->fDatabank->GetMetaDataFields());
		
		if (mf.size() > 0)
		{
			outText = fImpl->fDatabank->GetMetaData(inEntryID, mf[0].c_str());
			result = true;
		}
	}
	catch (exception& e)
	{
		outText.clear();
		gErrStr = e.what();
	}
	
	return result;
}

#ifndef NO_BLAST
bool MDatabank::ContainsBlastIndex() const
{
	bool hasBlastIndices = false;
	
	try
	{
		hasBlastIndices = fImpl->fDatabank->GetBlastDbCount() > 0;
	}
	catch (...)
	{
	}
	
	return hasBlastIndices;
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

bool MDatabank::Sequence(const string& inEntryID, unsigned long inIndex, string& outSequence)
{
	bool result = false;
	
	try
	{
		outSequence = fImpl->fDatabank->GetSequence(inEntryID, inIndex);
		result = true;
	}
	catch (exception& e)
	{
		outSequence.clear();
		gErrStr = e.what();
	}
	
	return result;
}

MBlastHits* MDatabank::Blast(const string& inQuery, const string& inMatrix,
	unsigned long inWordSize, double inExpect, bool inFilter,
	bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend)
{
	MBlastHits* result = nil;
	
	auto_ptr<CDocIterator> iter(new CDbAllDocIterator(fImpl->fDatabank->Count()));
	
	auto_ptr<MBlastHitsImp> impl(new MBlastHitsImp(*fImpl->fDatabank, inQuery, inMatrix,
		inWordSize, inExpect, inFilter, inGapped, inGapOpen, inGapExtend));
	
	if (impl->fBlast.Find(*fImpl->fDatabank, *iter))
	{
		impl->fHits.reset(new CBlastHitIterator(impl->fBlast.Hits()));
		result = MBlastHits::Create(impl.release());
	}
			
	return result;
}
#endif

MIndex* MDatabank::Index(const string& inIndex)
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

void MDatabank::SetStopWords(vector<string> inStopWords)
{
	fImpl->GetDB()->SetStopWords(inStopWords);
}

void MDatabank::Store(const string& inDocument)
{
	CStopwatch sw(fImpl->fStoreTime);
	fImpl->GetDB()->Store(inDocument);
}

void MDatabank::StoreMetaData(const string& inFieldName, const string& inValue)
{
	CStopwatch sw(fImpl->fStoreTime);
	fImpl->GetDB()->StoreMetaData(inFieldName, inValue);
}

void MDatabank::IndexText(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexText(inIndex, inText);
}

void MDatabank::IndexTextAndNumbers(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexTextAndNumbers(inIndex, inText);
}

void MDatabank::IndexWord(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexWord(inIndex, inText);
}

void MDatabank::IndexValue(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexValue(inIndex, inText);
}

void MDatabank::IndexWordWithWeight(const string& inIndex,
	const string& inText, unsigned long inFrequency)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexWordWithWeight(inIndex, inText, inFrequency);
}

void MDatabank::IndexDate(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexDate(inIndex, inText);
}

void MDatabank::IndexNumber(const string& inIndex, const string& inText)
{
	CStopwatch sw(fImpl->fIndexTime);
	fImpl->GetDB()->IndexNumber(inIndex, inText);
}

#ifndef NO_BLAST
void MDatabank::AddSequence(const string& inSequence)
{
	CStopwatch sw(fImpl->fStoreTime);
	fImpl->GetDB()->AddSequence(inSequence);
}
#endif

void MDatabank::FlushDocument()
{
	CStopwatch sw(fImpl->fFlushTime);
	fImpl->GetDB()->FlushDocument();
}

void MDatabank::Finish(bool inCreateAllTextIndex, bool inCreateUpdateDatabank)
{
	{
		CStopwatch sw(fImpl->fFinishTime);

		fImpl->GetDB()->Finish(inCreateAllTextIndex, inCreateUpdateDatabank);
		
		fImpl->Close();
		
		if (fImpl->fSafe.get())
			fImpl->fSafe->Commit();
	}

	// And now print out some statistics, if wanted of course
	if (VERBOSE > 1)
	{
		PrintStatistics();
		fImpl->PrintCreateStatistics();
	}
}

//void MDatabank::RecalcDocWeights(const string& inIndex)
//{
//	fImpl->GetDB()->RecalculateDocumentWeights(inIndex);
//}

// stupid swig...
// now we have to pass the indices contatenated as a string, separated by colon
void MDatabank::CreateDictionary(string inIndices, long inMinOccurrence, long inMinWordLength)
{
	vector<string> indices;
	
	string::size_type c;
	while ((c = inIndices.find(':')) != string::npos)
	{
		indices.push_back(inIndices.substr(0, c));
		inIndices.erase(0, c + 1);
	}
	indices.push_back(inIndices);
	
	fImpl->fDatabank->CreateDictionaryForIndexes(indices, inMinOccurrence, inMinWordLength);
}

MBooleanQuery* MDatabank::BooleanQuery(const string& inQuery)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);
	imp->fDatabank = fImpl->fDatabank.get();
	imp->fQuery.reset(new CParsedQueryObject(*fImpl->fDatabank, inQuery, false));
	return MBooleanQuery::Create(imp.release());
}

MRankedQuery* MDatabank::RankedQuery(const string& inIndex)
{
	auto_ptr<MRankedQueryImp> imp(new MRankedQueryImp);

	imp->fDatabank = fImpl->fDatabank.get();
	imp->fIndex = inIndex;
	imp->fAlgorithm = "vector";
	imp->fQuery.reset(new CRankedQuery);
	imp->fMaxReturn = 1000;
	imp->fAllTermsRequired = false;

	MRankedQuery* result = MRankedQuery::Create(imp.release());

	return result;
}

// ---------------------------------------------------------------------------
//
//  class MQueryResults
//

const char* MQueryResults::Next()
{
	const char* result = nil;
	uint32 docNr;
	
	if (fImpl->Next(docNr))
	{
		fImpl->fScratch = fImpl->fDatabank->GetDocumentID(docNr);
		result = fImpl->fScratch.c_str();
	}

	return result;
}

//bool MQueryResults::Next(string& outID, float& outScore)
//{
//	bool result = false;
//	uint32 docNr;
//	
//	if (fImpl->Next(docNr))
//	{
//		outID = fImpl->fDatabank->GetDocumentID(docNr);
//		outScore = fImpl->fScore;
//		result = true;
//	}
//
//	return result;
//}

unsigned long MQueryResults::Score() const
{
	return static_cast<unsigned long>(fImpl->fScore);
}

void MQueryResults::Skip(long inCount)
{
	if (inCount < 0)
		THROW(("Cannot skip backwards in query results"));
	
	uint32 docNr;
	while (inCount-- > 0)
		fImpl->Next(docNr);
}

unsigned long MQueryResults::Count(bool inExact) const
{
	return fImpl->Count(inExact);
}

#ifndef NO_BLAST
MBlastHits* MQueryResults::Blast(const string& inQuery, const string& inMatrix,
	unsigned long inWordSize, double inExpect, bool inFilter,
	bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend)
{
	MBlastHits* result = nil;
	
	auto_ptr<MBlastHitsImp> impl(new MBlastHitsImp(*fImpl->fDatabank, inQuery,
		inMatrix, inWordSize, inExpect, inFilter, inGapped, inGapOpen, inGapExtend));
	
	if (impl->fBlast.Find(*fImpl->fDatabank, *fImpl->fIter))
	{
		impl->fHits.reset(new CBlastHitIterator(impl->fBlast.Hits()));
		result = MBlastHits::Create(impl.release());
	}
			
	return result;
}
#endif


// ---------------------------------------------------------------------------
//
//  class MKeys
//

const char* MKeys::Next()
{
	const char* result = nil;
	int64 v;

	if (fImpl->fIter->Next(fImpl->fScratch, v))
		result = fImpl->fScratch.c_str();
	
	return result;
}

void MKeys::Skip(long inCount)
{
	string s;
	int64 v;

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

float MIndex::GetIDF(const string& inKey)
{
	auto_ptr<CDbDocIteratorBase> iter(
		fImpl->fDatabank->GetDocWeightIterator(Code(), inKey));
	
	if (iter.get() == nil)
		THROW(("No key %s in index %s", inKey.c_str(), Code().c_str()));
	
	return iter->GetIDFCorrectionFactor();
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


#ifndef NO_BLAST
// ---------------------------------------------------------------------------
//
//  class MBlastHit
//

const char* MBlastHit::Id()
{
	return fImpl->fId.c_str();
}

const char* MBlastHit::Title()
{
	return fImpl->fTitle.c_str();
}

MBlastHsps* MBlastHit::Hsps()
{
	auto_ptr<MBlastHspsImp> impl(new MBlastHspsImp);
	impl->fHsps.reset(new CBlastHspIterator(*fImpl->fHsps));
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

unsigned long MBlastHits::DbCount()
{
	return fImpl->fBlast.DbCount();
}

unsigned long MBlastHits::DbLength()
{
	return fImpl->fBlast.DbLength();
}

unsigned long MBlastHits::EffectiveSpace()
{
	return fImpl->fBlast.EffectiveSpace();
}

double MBlastHits::Kappa()
{
	return fImpl->fBlast.Kappa();
}

double MBlastHits::Lambda()
{
	return fImpl->fBlast.Lambda();
}

double MBlastHits::Entropy()
{
	return fImpl->fBlast.Entropy();
}

MBlastHit* MBlastHits::Next()
{
	MBlastHit* result = nil;
	
	if (fImpl->fHits->Next())
	{
		auto_ptr<MBlastHitImp> impl(new MBlastHitImp);
		impl->fId = fImpl->fHits->DocumentID();
		impl->fTitle = fImpl->fDb.GetMetaData(impl->fId, "title");
		impl->fHsps.reset(new CBlastHspIterator(fImpl->fHits->Hsps()));
		result = MBlastHit::Create(impl.release());
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

unsigned long MBlastHsp::Identity()
{
	return fImpl->fIdentity;
}

unsigned long MBlastHsp::Positive()
{
	return fImpl->fPositive;
}

unsigned long MBlastHsp::Gaps()
{
	return fImpl->fGaps;
}

unsigned long MBlastHsp::QueryStart()
{
	return fImpl->fQueryStart;
}

unsigned long MBlastHsp::SubjectStart()
{
	return fImpl->fSubjectStart;
}

unsigned long MBlastHsp::SubjectLength()
{
	return fImpl->fSubjectLength;
}

string MBlastHsp::QueryAlignment()
{
	return fImpl->fQueryAlignment;
}

string MBlastHsp::SubjectAlignment()
{
	return fImpl->fSubjectAlignment;
}

string MBlastHsp::Midline()
{
	return fImpl->fMidline;
}

// ---------------------------------------------------------------------------
//
//  class MBlastHsps
//

MBlastHsp* MBlastHsps::Next()
{
	MBlastHsp* result = nil;
	
	if (fImpl->fHsps->Next())
	{
		auto_ptr<MBlastHspImp> impl(new MBlastHspImp);

		impl->fQueryStart = fImpl->fHsps->QueryStart();
		impl->fSubjectStart = fImpl->fHsps->SubjectStart();
		impl->fSubjectLength = fImpl->fHsps->SubjectLength();
		impl->fQueryAlignment = fImpl->fHsps->QueryAlignment();
		impl->fSubjectAlignment = fImpl->fHsps->SubjectAlignment();
		impl->fMidline = fImpl->fHsps->Midline();
		impl->fScore = fImpl->fHsps->Score();
		impl->fBitScore = fImpl->fHsps->BitScore();
		impl->fExpect = fImpl->fHsps->Expect();
		impl->fIdentity = fImpl->fHsps->Identity();
		impl->fPositive = fImpl->fHsps->Positive();
		impl->fGaps = fImpl->fHsps->Gaps();

		result = MBlastHsp::Create(impl.release());
	}
	
	return result;
}
#endif

// ---------------------------------------------------------------------------
//
//  class MBooleanQuery
//

MBooleanQuery* MBooleanQuery::Not(MBooleanQuery* inQuery)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);
	imp->fDatabank = inQuery->fImpl->fDatabank;
	imp->fQuery.reset(new CNotQueryObject(*imp->fDatabank, inQuery->fImpl->fQuery));
	return MBooleanQuery::Create(imp.release());
}

MBooleanQuery* MBooleanQuery::Union(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);

	if (inQueryA->fImpl->fDatabank != inQueryB->fImpl->fDatabank)
		THROW(("Databanks should be the same for union boolean query objects"));

	imp->fDatabank = inQueryA->fImpl->fDatabank;
	imp->fQuery.reset(new CUnionQueryObject(*imp->fDatabank,
		inQueryA->fImpl->fQuery, inQueryB->fImpl->fQuery));
	return MBooleanQuery::Create(imp.release());
}

MBooleanQuery* MBooleanQuery::Intersection(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB)
{
	auto_ptr<MBooleanQueryImp> imp(new MBooleanQueryImp);

	if (inQueryA->fImpl->fDatabank != inQueryB->fImpl->fDatabank)
		THROW(("Databanks should be the same for intersection boolean query objects"));

	imp->fDatabank = inQueryA->fImpl->fDatabank;
	imp->fQuery.reset(new CIntersectionQueryObject(*imp->fDatabank,
		inQueryA->fImpl->fQuery, inQueryB->fImpl->fQuery));
	return MBooleanQuery::Create(imp.release());
}

void MBooleanQuery::Prefetch()
{
	fImpl->fQuery->Prefetch();
}

MQueryResults* MBooleanQuery::Perform()
{
	auto_ptr<MQueryResultsImp> imp(
		new MQueryResultsImp(*fImpl->fDatabank, fImpl->fQuery->Perform()));

	MQueryResults* result = NULL;

	if (imp->Count(false) > 0)
		result = MQueryResults::Create(imp.release());

	return result;
}

// ---------------------------------------------------------------------------
//
//  class MRankedQuery
//

void MRankedQuery::AddTerm(const string& inTerm, unsigned long inFrequency)
{
	fImpl->fQuery->AddTerm(inTerm, inFrequency);
}

void MRankedQuery::AddTermsFromText(const string& inText)
{
	fImpl->fQuery->AddTermsFromText(inText);
}

void MRankedQuery::SetAlgorithm(const string& inAlgorithm)
{
	fImpl->fAlgorithm = inAlgorithm;
}

void MRankedQuery::SetMaxReturn(int inMaxReturn)
{
	fImpl->fMaxReturn = inMaxReturn;
}

void MRankedQuery::SetAllTermsRequired(bool inAllTermsRequired)
{
	fImpl->fAllTermsRequired = inAllTermsRequired;
}

MQueryResults* MRankedQuery::Perform(MBooleanQuery* inMetaQuery)
{
	auto_ptr<CDocIterator> metaDocs;
	if (inMetaQuery != nil)
		metaDocs.reset(inMetaQuery->fImpl->fQuery->Perform());
	
	MQueryResults* result = nil;

	if (inMetaQuery == nil or metaDocs.get() != nil)
	{
		CDocIterator* data = nil;
		uint32 count;
		
		fImpl->fQuery->PerformSearch(
			*fImpl->fDatabank, fImpl->fIndex, fImpl->fAlgorithm, metaDocs.release(),
			fImpl->fMaxReturn, fImpl->fAllTermsRequired,
			data, count);
		
		auto_ptr<MQueryResultsImp> imp(new MQueryResultsImp(*fImpl->fDatabank, data, count));
	
		if (imp->Count(false) > 0)
			result = MQueryResults::Create(imp.release());
	}

	return result;
}

//// Instantiate destructor objects
//
//MRSObject<MBlastHits, MBlastHitsImp>::MRSObject();
//MRSObject<MBlastHsps, MBlastHspsImp>::MRSObject();
//MRSObject<MRankedQuery, MRankedQueryImp>::MRSObject();
//MRSObject<MBooleanQuery, MBooleanQueryImp>::MRSObject();
//MRSObject<MQueryResults, MQueryResultsImp>::MRSObject();
//MRSObject<MKeys, MKeysImp>::MRSObject();
//MRSObject<MIndex, MIndexImp>::MRSObject();
//MRSObject<MIndices, MIndicesImp>::MRSObject();
//MRSObject<MBlastHit, MBlastHitImp>::MRSObject();
//MRSObject<MBlastHsp, MBlastHspImp>::MRSObject();
template class MRSObject<MDatabank, MDatabankImp>::MRSObject;

#pragma export off
