/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
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

#include "HFile.h"
#include "HError.h"
#include "HStream.h"
#include <iostream>
#include <deque>
#include <set>
#include <fcntl.h>
#include "HUtils.h"
#include "HByteSwap.h"
#include "HStream.h"
#include "HUtils.h"
#include "zlib.h"
#include <uuid/uuid.h>

#include "CDatabank.h"
#include "CCompress.h"
#include "CDecompress.h"
#include "CIndexer.h"
#include "CIndex.h"
#include "CUtils.h"
#include "CBitStream.h"
#include "CDbInfo.h"
#include "CArray.h"
#include "CIdTable.h"
#ifndef NO_BLAST
#include "CBlastIndex.h"
#endif
#include "CDictionary.h"
#include "CDocWeightArray.h"

using namespace std;

/*
	Data file format related structures
*/

const uint32
	kHeaderSig = FOUR_CHAR_INLINE('MRSd'),
	kDataSig = FOUR_CHAR_INLINE('data'),
	kPartSig = FOUR_CHAR_INLINE('part'),
	kBlastIndexSignature = FOUR_CHAR_INLINE('blst');

struct SHeader
{
	uint32		sig;
	uint32		size;
	int64		entries;
	int64		data_offset;
	int64		data_size;
	int64		index_offset;
	int64		index_size;
	int64		info_offset;
	int64		info_size;
	int64		id_offset;
	int64		id_size;
	int64		blast_ix_offset;
	int64		blast_ix_size;
	uuid_t		uuid;
};

struct SDataHeader
{
	uint32			sig;
	uint32			size;
	uint32			count;
};

struct SDataPart
{
	uint32			sig;
	uint32			size;
	int64			data_offset;	// from beginning of file
	int64			data_size;		
	int64			table_offset;	// from beginning of file
	int64			table_size;
	uint32			count;			// number of entries in this part
	uint32			kind;			// compression kind
};

struct SBlastIndexHeader
{
	uint32			sig;
	uint32			size;
	int64			data_offset;	// from beginning of file
	int64			data_size;		
	int64			table_offset;	// from beginning of file
	int64			table_size;
	int64			db_length;		// the total sequence length
	uint32			seq_count;		// the total number of sequences
	uint32			count;			// number of entries in this part
	uint32			kind;			// compression kind
};

HStreamBase& operator<<(HStreamBase& inData, SHeader& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SHeader& inStruct);
HStreamBase& operator<<(HStreamBase& inData, SDataHeader& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SDataHeader& inStruct);
HStreamBase& operator<<(HStreamBase& inData, SDataPart& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SDataPart& inStruct);
HStreamBase& operator<<(HStreamBase& inData, SBlastIndexHeader& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SBlastIndexHeader& inStruct);

/*
	Base classes
*/

CDatabankBase::CDatabankBase()
	: fDictionary(nil)
{
}

CDatabankBase::~CDatabankBase()
{
	delete fDictionary;
}

string CDatabankBase::GetDbName() const
{
	return kEmptyString;
}

//void CDatabankBase::DumpIndex(const string& inIndex)
//{
//}

CIteratorBase* CDatabankBase::GetIteratorForIndex(const string& inIndex)
{
    return nil;
}

CIteratorBase* CDatabankBase::GetIteratorForIndexAndKey(
	const string& inIndex, const string& inKey)
{
    return nil;
}

CDbDocIteratorBase* CDatabankBase::GetDocWeightIterator(
	const string& inIndex, const string& inKey)
{
	return nil;
}

bool CDatabankBase::GetDocumentNr(const std::string& inDocID, uint32& outDocNr) const
{
	stringstream s(inDocID);
	s >> outDocNr;
	return outDocNr < Count();
}

uint32 CDatabankBase::GetDocumentNr(const std::string& inDocID) const
{
	uint32 result;
	if (not GetDocumentNr(inDocID, result))
		THROW(("Document (%s) not found", inDocID.c_str()));
	return result;
}

string CDatabankBase::GetDocument(const string& inDocumentID)
{
	return GetDocument(GetDocumentNr(inDocumentID));
}

string CDatabankBase::GetDocumentID(uint32 inDocNr) const
{
	stringstream s;
	s << inDocNr;
	return s.str();
}

#ifndef NO_BLAST
string CDatabankBase::GetSequence(uint32 inDocNr, uint32 inIndex)
{
	CSequence seq;
	GetSequence(inDocNr, inIndex, seq);
	return Decode(seq);
}

string CDatabankBase::GetSequence(const string& inDocID, uint32 inIndex)
{
	CSequence seq;
	GetSequence(GetDocumentNr(inDocID), inIndex, seq);
	return Decode(seq);
}
#endif

// lame... but works
uint32 CDatabankBase::GetIndexNr(const string& inIndexName)
{
	uint32 result = 0;

	while (result < GetIndexCount())
	{
		string code, type;
		uint32 count;
		GetIndexInfo(result, code, type, count);
		if (code == inIndexName)
			break;
		++result;
	}
	
	return result;
}

std::string	CDatabankBase::GetDbNameForDocID(const std::string& inDocID) const
{
	return GetDbName();
}

HUrl CDatabankBase::GetDbDirectory() const
{
	HUrl result;
	result.SetNativePath(getenv("MRS_DATA_DIR"));
	return result;
}

HUrl CDatabankBase::GetWeightFileURL(const string& inIndex) const
{
	HUrl url = GetDbDirectory();
	return url.GetChild(GetDbName() + '_' + inIndex + ".weights");
}

HUrl CDatabankBase::GetDictionaryFileURL() const
{
	HUrl url = GetDbDirectory();
	return url.GetChild(GetDbName() + ".dict");
}

void CDatabankBase::RecalculateDocumentWeights(const string& inIndex)
{
	if (VERBOSE > 0)
		cout << "Recalculating document weights for index " << inIndex << "... ";
	
	HFile::SafeSaver save(GetWeightFileURL(inIndex));
	
	int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
	auto_ptr<HStreamBase> file(new HFileStream(save.GetURL(), mode));
	
	uint32 docCount = Count();
	
	HAutoBuf<float> dwb(new float[docCount]);
	float* dw = dwb.get();
	
	memset(dw, 0, docCount * sizeof(float));
	
	auto_ptr<CIteratorBase> keys(GetIteratorForIndex(inIndex));
	string key;
	int64 v;
	
	while (keys->Next(key, v))
	{
		auto_ptr<CDbDocIteratorBase> iter(GetDocWeightIterator(inIndex, key));
		
		float idf = iter->GetIDFCorrectionFactor();
		
		uint32 doc;
		uint8 freq;

		while (iter->Next(doc, freq, false))
		{
			float wdt = freq * idf;
			dw[doc] += wdt * wdt;
		}
	}
	
	for (uint32 d = 0; d < docCount; ++d)
	{
		if (dw[d] != 0)
		{
			dw[d] = sqrt(dw[d]);
#if P_LITTLEENDIAN
			dw[d] = byte_swapper::swap(dw[d]);
#endif
		}
	}
	
	file->Write(dw, docCount * sizeof(float));
	
	if (VERBOSE > 0)
		cout << "done" << endl;
	
	save.Commit();
}

CDocWeightArray CDatabankBase::GetDocWeights(const std::string& inIndex)
{
	HUrl url = GetWeightFileURL(inIndex);
	
	if (not HFile::Exists(url))
		RecalculateDocumentWeights(inIndex);
	
	CDocWeightArray arr(url, Count());
	return arr;
}

void CDatabankBase::CreateDictionaryForIndexes(const vector<string>& inIndexNames,
	uint32 inMinOccurrence, uint32 inMinWordLength)
{
	delete fDictionary;
	fDictionary = nil;

	CDictionary::Create(*this, inIndexNames, inMinOccurrence, inMinWordLength);
}

vector<string> CDatabankBase::SuggestCorrection(const string& inKey)
{
	if (fDictionary == nil)
		fDictionary = new CDictionary(*this);

	return fDictionary->SuggestCorrection(inKey);
}

/*
	Implementations
*/

CDatabank::CDatabank(const HUrl& inUrl, bool inNew)
	: fPath(inUrl)
	, fDataFile(NULL)
	, fCompressor(NULL)
	, fIndexer(NULL)
	, fReadOnly(not inNew)
	, fInfoContainer(nil)
	, fIdTable(nil)
#ifndef NO_BLAST
	, fBlastIndex(nil)
#endif
	, fHeader(new SHeader)
	, fDataHeader(new SDataHeader)
	, fParts(nil)
#ifndef NO_BLAST
	, fBlast(nil)
#endif
{
	memset(fHeader, 0, sizeof(SHeader));
	memset(fDataHeader, 0, sizeof(SDataHeader));
	
	if (inNew)
	{
		int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
		
		fDataFile = new HBufferedFileStream(fPath, mode);
		
		fHeader->sig = kHeaderSig;
		
		*fDataFile << *fHeader;

		fHeader->data_offset = fDataFile->Tell();
		
		fDataHeader->sig = kDataSig;
		fDataHeader->count = 1;
		*fDataFile << *fDataHeader;
		
		fParts = new SDataPart[1];
		memset(fParts, 0, sizeof(SDataPart));
		fParts[0].sig = kPartSig;
		*fDataFile << fParts[0];
		
		fCompressor = new CCompressor(*fDataFile, inUrl);
		fIndexer = new CIndexer(inUrl);

//		fInfoContainer = new CDbInfo;
	}
	else
	{
		int mode = O_RDONLY | O_BINARY;
		
		fDataFile = new HBufferedFileStream(fPath, mode);
		
		*fDataFile >> *fHeader;
	
		if (fHeader->sig != kHeaderSig)
			THROW(("Not a mrs data file"));
		
		if (fHeader->info_size > 0)
		{
			HStreamView s(*fDataFile, fHeader->info_offset, fHeader->info_size);
			fInfoContainer = new CDbInfo(s);
		}
	
		if (fHeader->id_size > 0)
			fIdTable = new CIdTable(*fDataFile, fHeader->id_offset, fHeader->id_size);

		fDataFile->Seek(fHeader->data_offset, SEEK_SET);
		(*fDataFile) >> *fDataHeader;
		
		fParts = new SDataPart[fDataHeader->count];
		
		uint32 i;
		for (i = 0; i < fDataHeader->count; ++i)
			*fDataFile >> fParts[i];
		
		for (i = 0; i < fDataHeader->count; ++i)
		{
			SDataPart& pi = fParts[i];
			
			CDataPart cpi;
			cpi.fDecompressor = nil;
			cpi.fCount = pi.count;
			fDataParts.push_back(cpi);
		}
		
#ifndef NO_BLAST
		if (fHeader->blast_ix_size > 0)
		{
			fBlast = new SBlastIndexHeader;
			
			fDataFile->Seek(fHeader->blast_ix_offset, SEEK_SET);
			(*fDataFile) >> *fBlast;
			
			if (fBlast->sig != kBlastIndexSignature /*or fBlast->size < sizeof(SBlastIndexHeader) */)
				THROW(("Unknown or corrupt blast index"));
			
			fBlastIndex = new CBlastIndex(*fDataFile, fBlast->kind,
				fBlast->data_offset, fBlast->data_size,
				fBlast->table_offset, fBlast->table_size);
		}
#endif
	}
}

CDatabank::~CDatabank()
{
	for (CPartList::iterator i = fDataParts.begin(); i != fDataParts.end(); ++i)
		delete (*i).fDecompressor;
	
	delete[] fParts;
	delete fDataHeader;
	delete fHeader;
	
	delete fDataFile;
	delete fCompressor;
	delete fIndexer;
	delete fInfoContainer;
	delete fIdTable;
#ifndef NO_BLAST
	delete fBlast;
	delete fBlastIndex;
#endif
}

string CDatabank::GetDbName() const
{
	return basename(fPath.GetFileName());
}

void CDatabank::Finish(bool inCreateAllTextIndex)
{
	assert(fCompressor->Count() == fHeader->entries);
	assert(fIndexer->Count() == fHeader->entries);

	if (VERBOSE >= 1)
		cout << endl << "Finishing" << endl;

	if (fCompressor == nil or fHeader->entries == 0)
		THROW(("No data processed, cannot continue"));
	
	assert(fParts);
	
	SDataPart& partInfo = fParts[0];
	
	fCompressor->Finish(partInfo.data_offset, partInfo.data_size,
		partInfo.table_offset, partInfo.table_size,
		partInfo.kind, partInfo.count);
	
	fHeader->data_size = fDataFile->Tell() - fHeader->data_offset;
	
	fDataFile->Seek(fHeader->data_offset + sizeof(SDataHeader), SEEK_SET);
	*fDataFile << partInfo;

	delete fCompressor;
	fCompressor = NULL;
	
	fDataFile->Seek(0, SEEK_END);
	
	fIndexer->CreateIndex(*fDataFile, fHeader->index_offset,
		fHeader->index_size, inCreateAllTextIndex);
	delete fIndexer;
	fIndexer = nil;
	fIndexer = GetIndexer();

	auto_ptr<CIteratorBase> iter(fIndexer->GetIteratorForIndex("id"));
	if (iter.get() != nil)
	{
		if (VERBOSE >= 1)
		{
			cout << "Creating ID table... ";
			cout.flush();
		}
	
		fDataFile->Seek(0, SEEK_END);
		fHeader->id_offset = fDataFile->Tell();
		CIdTable::Create(*fDataFile, *iter.get(), fHeader->entries);
		fHeader->id_size = fDataFile->Tell() - fHeader->id_offset;

		if (VERBOSE >= 1)
			cout << "done" << endl;
	}
	else if (VERBOSE >= 1)
		cout << "No ID table created since there is no id index" << endl;

//	fIndexer->FixupDocWeights();

	iter.release();

	delete fIndexer;
	fIndexer = nil;
	
	if (VERBOSE >= 1)
		cout << "Index finished" << endl;

	if (fInfoContainer != nil)
	{
		fDataFile->Seek(0, SEEK_END);
		fHeader->info_offset = fDataFile->Tell();
		fInfoContainer->Write(*fDataFile);
		fHeader->info_size = fDataFile->Tell() - fHeader->info_offset;
	}
	
#ifndef NO_BLAST
	if (fBlastIndex != nil)
	{
		fDataFile->Seek(0, SEEK_END);

		fHeader->blast_ix_offset = fDataFile->Tell();
		
		SBlastIndexHeader bh = { kBlastIndexSignature, 0 };
		
		(*fDataFile) << bh;

		fBlastIndex->Finish(*fDataFile, bh.data_offset, bh.data_size,
			bh.table_offset, bh.table_size, bh.kind, bh.count,
			bh.db_length, bh.seq_count);
		
		delete fBlastIndex;
		fBlastIndex = nil;

		fHeader->blast_ix_size = fDataFile->Tell() - fHeader->blast_ix_offset;
		
		fDataFile->Seek(fHeader->blast_ix_offset, SEEK_SET);
		(*fDataFile) << bh;
		
		fDataFile->Seek(0, SEEK_END);
	}
#endif
	
	fDataFile->Seek(0, SEEK_SET);
	*fDataFile << *fHeader;

	delete fDataFile;
	fDataFile = nil;

	if (VERBOSE >= 1)
		cout << "Datafile closed" << endl;
}

void CDatabank::Merge(vector<CDatabank*>& inParts)
{
	if (fReadOnly)
	{
		delete fDataFile;
		int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
		fDataFile = new HBufferedFileStream(fPath, mode);
	}

	fDataFile->Seek(0, SEEK_SET);
	*fDataFile << *fHeader;

	fHeader->sig = kHeaderSig;
	fHeader->data_offset = fDataFile->Tell();
	fHeader->entries = 0;
	
	// first copy over all the data

	vector<CDatabank*>::iterator d;
	
	fDataHeader->sig = kDataSig;
	
	fDataHeader->count = 0;
	for (d = inParts.begin(); d != inParts.end(); ++d)
		fDataHeader->count += (*d)->fDataParts.size();

	if (fParts != nil)
		delete[] fParts;

	fParts = new SDataPart[fDataHeader->count];
	memset(fParts, 0, sizeof(SDataPart) * fDataHeader->count);

		// reserve space
	(*fDataFile) << *fDataHeader;
	for (uint32 ix = 0; ix < fDataHeader->count; ++ix)
		*fDataFile << fParts[ix];
	
	uint32 part = 0;
	bool hasBlastIndices = false;

	for (d = inParts.begin(); d != inParts.end(); ++d)
	{
#ifndef NO_BLAST
		hasBlastIndices = hasBlastIndices or ((*d)->GetBlastIndex() != nil);
#endif
		
		if (VERBOSE >= 1)
		{
			cout << "Copying data from " << (*d)->fPath.GetFileName() << " ";
			cout.flush();
		}

		for (uint32 ix = 0; ix < (*d)->fDataHeader->count; ++ix)
		{
			if (VERBOSE >= 1)
			{
				cout << ".";
				cout.flush();
			}
			
			if ((*d)->fParts[ix].kind == 0)
				continue;
			
			(*d)->GetDecompressor(ix)->CopyData(*fDataFile, fParts[part].kind,
				fParts[part].data_offset, fParts[part].data_size,
				fParts[part].table_offset, fParts[part].table_size);

			fParts[part].count = (*d)->fParts[ix].count;
			fParts[part].sig = kPartSig;

			++part;
			fHeader->entries += (*d)->fParts[ix].count;
		}

		if (VERBOSE >= 1)
			cout << " done" << endl;
	}

	fHeader->data_size = fDataFile->Tell() - fHeader->data_offset;
	
	fDataFile->Seek(fHeader->data_offset, SEEK_SET);
	(*fDataFile) << *fDataHeader;

	for (uint32 i = 0; i < fDataHeader->count; ++i)
		(*fDataFile) << fParts[i];

	if (VERBOSE >= 1)
		cout << "done" << endl;

	// Merge indices
	
	fHeader->index_offset = fDataFile->Seek(0, SEEK_END);
	CIndexer index(fPath);
	index.MergeIndices(*fDataFile, inParts);
	fDataFile->Seek(0, SEEK_END);
	fHeader->index_size = fDataFile->Tell() - fHeader->index_offset;

	auto_ptr<CIteratorBase> iter(index.GetIteratorForIndex("id"));
	if (iter.get() != nil)
	{
		if (VERBOSE >= 1)
		{
			cout << "Creating ID table... ";
			cout.flush();
		}
		
		fDataFile->Seek(0, SEEK_END);
		fHeader->id_offset = fDataFile->Tell();
		CIdTable::Create(*fDataFile, *iter.get(), fHeader->entries);
		fDataFile->Seek(0, SEEK_END);
		fHeader->id_size = fDataFile->Tell() - fHeader->id_offset;

		if (VERBOSE >= 1)
			cout << "done" << endl;
		
		iter.release();
	}
	else if (VERBOSE >= 1)
		cout << "No ID table created since there is no id index" << endl;

#ifndef NO_BLAST
	// Merge blast index

	if (hasBlastIndices)
	{
		fDataFile->Seek(0, SEEK_END);

		fHeader->blast_ix_offset = fDataFile->Tell();
		
		SBlastIndexHeader bh = { kBlastIndexSignature, 0 };
		
		(*fDataFile) << bh;
		
		CBlastIndex blastIndex(fPath);
		blastIndex.MergeIndices(inParts);
		blastIndex.Finish(*fDataFile, bh.data_offset, bh.data_size,
			bh.table_offset, bh.table_size, bh.kind, bh.count,
			bh.db_length, bh.seq_count);
		
		fHeader->blast_ix_size = fDataFile->Tell() - fHeader->blast_ix_offset;
		
		fDataFile->Seek(fHeader->blast_ix_offset, SEEK_SET);
		(*fDataFile) << bh;
		
		fDataFile->Seek(0, SEEK_END);
	}		
#endif

	// Merge info
	
	fInfoContainer = new CDbInfo;
	
	for (d = inParts.begin(); d != inParts.end(); ++d)
	{
		if ((*d)->fInfoContainer == nil)
			continue;
		
		if (VERBOSE >= 1)
		{
			cout << "Copying info from " << (*d)->fPath.GetFileName() << " ";
			cout.flush();
		}

		uint32 cookie = 0;
		string s;
		uint32 kind;
		
		while ((*d)->fInfoContainer->Next(cookie, s, kind))
			fInfoContainer->Add(kind, s);

		if (VERBOSE >= 1)
			cout << " done" << endl;
	}
	
	if (fInfoContainer != nil)
	{
		fDataFile->Seek(0, SEEK_END);
		fHeader->info_offset = fDataFile->Tell();
		fInfoContainer->Write(*fDataFile);
		fDataFile->Seek(0, SEEK_END);
		fHeader->info_size = fDataFile->Tell() - fHeader->info_offset;
	}

	// Write the header
	
	fDataFile->Seek(0, SEEK_SET);
	*fDataFile << *fHeader;

	// now fix up the weighted document weights...
//	fIndexer->FixupDocWeights();
}

string CDatabank::GetDocument(uint32 inDocNr)
{
	CPartList::iterator p = fDataParts.begin();
	while (p != fDataParts.end() and inDocNr >= (*p).fCount)
	{
		inDocNr -= (*p).fCount;
		++p;
	}
	
	if (p == fDataParts.end())
		THROW(("Doc number out of range"));
	
	return GetDecompressor(p - fDataParts.begin())
		->GetDocument(inDocNr);
}

bool CDatabank::GetDocumentNr(const string& inDocID, uint32& outDocNr) const
{
	if (fIndexer == nil)
		fIndexer = new CIndexer(*fDataFile,
			fHeader->index_offset, fHeader->index_size);
	
	return fIndexer->GetDocumentNr(inDocID, outDocNr);
}

#ifndef NO_BLAST
void CDatabank::GetSequence(uint32 inDocNr, uint32 inIndex,
	CSequence& outSequence)
{
	if (fBlastIndex == nil)
		THROW(("No blast index available for this databank"));
	
	outSequence = fBlastIndex->GetSequence(inDocNr, inIndex);
}

uint32 CDatabank::CountSequencesForDocument(uint32 inDocNr)
{
	if (fBlastIndex == nil)
		THROW(("No blast index available for this databank"));
	
	return fBlastIndex->CountSequencesForDocument(inDocNr);
}

uint32 CDatabank::GetBlastDbCount() const
{
	if (fBlast == nil)
		THROW(("No blast index available for this databank"));
	
	return fBlast->seq_count;
}

int64 CDatabank::GetBlastDbLength() const
{
	if (fBlast == nil)
		THROW(("No blast index available for this databank"));
	
	return fBlast->db_length;
}
#endif

CDecompressor* CDatabank::GetDecompressor(uint32 inPartNr)
{
	if (inPartNr >= fDataHeader->count)
		THROW(("runtime error: PartNr out of range"));

	if (fDataParts[inPartNr].fDecompressor == nil)
	{
		fDataParts[inPartNr].fDecompressor =
			new CDecompressor(*fDataFile, fParts[inPartNr].kind,
				fParts[inPartNr].data_offset, fParts[inPartNr].data_size,
				fParts[inPartNr].table_offset, fParts[inPartNr].table_size);
	}
	
	return fDataParts[inPartNr].fDecompressor;
}

uint32 CDatabank::Count() const
{
	return fHeader->entries;
}

string CDatabank::GetVersion() const
{
	string vers, s;
	uint32 cookie = 0, k;
	
	while (fInfoContainer and fInfoContainer->Next(cookie, s, k, 'vers'))
	{
		if (vers.length())
			vers += '\t';
		vers += s;
	}
	
	if (vers.length() == 0)
		vers = "no version information available";
	
	return vers;
}

string CDatabank::GetDocumentID(uint32 inDocNr) const
{
	string result;
	
	if (fIdTable != nil)
		result = fIdTable->GetID(inDocNr);
	else
		result = CDatabankBase::GetDocumentID(inDocNr);
	
	return result;
}

CIndexer* CDatabank::GetIndexer()
{
	if (fIndexer == nil)
		fIndexer = new CIndexer(*fDataFile,
			fHeader->index_offset, fHeader->index_size);
	return fIndexer;
}

long CDatabank::GetIndexCount()
{
	return GetIndexer()->GetIndexCount();
}

void CDatabank::GetIndexInfo(uint32 inIndexNr, string& outCode,
	string& outType, uint32& outCount)
{
	return GetIndexer()->GetIndexInfo(inIndexNr, outCode, outType, outCount);
}

void CDatabank::DumpIndex(const string& inIndex)
{
	return GetIndexer()->DumpIndex(inIndex);
}

CIteratorBase* CDatabank::GetIteratorForIndex(const string& inIndex)
{
	return GetIndexer()->GetIteratorForIndex(inIndex);
}

CIteratorBase* CDatabank::GetIteratorForIndexAndKey(
	const string& inIndex, const string& inKey)
{
	return GetIndexer()->GetIteratorForIndexAndKey(inIndex, inKey);
}

CDbDocIteratorBase* CDatabank::GetDocWeightIterator(
	const string& inIndex, const string& inKey)
{
	return GetIndexer()->GetDocWeightIterator(inIndex, inKey);
}

uint32 CDatabank::CountDocumentsContainingKey(
	const string& inIndex, const string& inKey)
{
	auto_ptr<CDocIterator> iter(GetIndexer()->CreateDocIterator(inIndex, inKey, false, kOpContains));
	
	uint32 result = 0;
	if (iter.get())
		result = iter->Count();
	
	return result;
}

void CDatabank::PrintInfo()
{
//	string p(fPath.GetURL());
//	HUrl::DecodeReservedChars(p);
	
	cout << "File: " << fPath.GetNativePath() << endl;
	cout << endl;
	
	const char* sig = reinterpret_cast<const char*>(&fHeader->sig);

	cout << "Header:" << endl;
	cout << "  signature:    " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
	cout << "  size:         " << fHeader->size << endl;
	cout << "  entries:      " << fHeader->entries << endl;
	cout << "  data offset:  " << fHeader->data_offset << endl;
	cout << "  data size:    " << fHeader->data_size << endl;
	cout << "  index offset: " << fHeader->index_offset << endl;
	cout << "  index size:   " << fHeader->index_size << endl;
	cout << "  info offset:  " << fHeader->info_offset << endl;
	cout << "  info size:    " << fHeader->info_size << endl;
	cout << "  id offset:    " << fHeader->id_offset << endl;
	cout << "  id size:      " << fHeader->id_size << endl;
#ifndef NO_BLAST
	cout << "  blast offset: " << fHeader->blast_ix_offset << endl;
	cout << "  blast size:   " << fHeader->blast_ix_size << endl;
#endif
	cout << endl;
	
	sig = reinterpret_cast<const char*>(&fDataHeader->sig);
	
	cout << "Data Header:" << endl;
	cout << "  signature:    " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
	cout << "  size:         " << fDataHeader->size << endl;
	cout << "  count :       " << fDataHeader->count << endl;
	cout << endl;
	
	for (uint32 ix = 0; ix < fDataHeader->count; ++ix)
	{
		SDataPart& p = fParts[ix];
		
		sig = reinterpret_cast<const char*>(&p.sig);
		
		cout << "Data Part " << ix << ":" << endl;
		cout << "  signature:    " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
		cout << "  size:         " << p.size << endl;
		cout << "  data offset:  " << p.data_offset << endl;
		cout << "  data size:    " << p.data_size << endl;
		cout << "  table offset: " << p.table_offset << endl;
		cout << "  table size:   " << p.table_size << endl;
		cout << "  count:        " << p.count << endl;

		sig = reinterpret_cast<const char*>(&p.kind);
		cout << "  compression:  " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
		cout << endl;
	}
	
	if (fIndexer == nil)
		fIndexer = new CIndexer(*fDataFile,
			fHeader->index_offset, fHeader->index_size);

	fIndexer->PrintInfo();
	
	if (fIdTable != nil)
		fIdTable->PrintInfo();
	
#ifndef NO_BLAST
	if (fBlast != nil)
	{
		
		
	}
#endif
}

void CDatabank::Store(const string& inDocument)
{
	fCompressor->AddDocument(inDocument.c_str(), inDocument.length());
	++fHeader->entries;
}

void CDatabank::IndexText(const string& inIndex, const string& inText)
{
	fIndexer->IndexText(inIndex, inText);
}

void CDatabank::IndexDate(const string& inIndex, const string& inText)
{
	fIndexer->IndexDate(inIndex, inText);
}

void CDatabank::IndexTextAndNumbers(const string& inIndex, const string& inText)
{
	fIndexer->IndexTextAndNumbers(inIndex, inText);
}

void CDatabank::IndexNumber(const string& inIndex, const string& inText)
{
	fIndexer->IndexNumber(inIndex, inText);
}

void CDatabank::IndexWord(const string& inIndex, const string& inText)
{
	fIndexer->IndexWord(inIndex, inText);
}

void CDatabank::IndexValue(const string& inIndex, const string& inText)
{
	fIndexer->IndexValue(inIndex, inText);
}

void CDatabank::IndexWordWithWeight(const string& inIndex,	
	const string& inText, uint32 inFrequency)
{
	fIndexer->IndexWordWithWeight(inIndex, inText, inFrequency);
}

#ifndef NO_BLAST
void CDatabank::AddSequence(const string& inSequence)
{
	if (fBlastIndex == nil)
	{
		fBlastIndex = new CBlastIndex(fPath);
		while (fBlastIndex->Count() < fIndexer->Count())
			fBlastIndex->FlushDoc();
	}
		
	fBlastIndex->AddSequence(inSequence);
}
#endif

void CDatabank::FlushDocument()
{
	fIndexer->FlushDoc();
#ifndef NO_BLAST
	if (fBlastIndex)
		fBlastIndex->FlushDoc();
#endif

	if (VERBOSE >= 1 and (fIndexer->Count() % 1000) == 0)
	{
		cout << ".";
		cout.flush();
	}
}

void CDatabank::SetVersion(const string& inVersion)
{
	if (fInfoContainer == nil)
		fInfoContainer = new CDbInfo;
	fInfoContainer->Add('vers', inVersion);
}

CDocIterator* CDatabank::CreateDocIterator(const string& inIndex,
	const string& inKey, bool inKeyIsPattern, CQueryOperator inOperator)
{
	CDocIterator* result;
	if (inIndex == "*" and inKey == "*" and inKeyIsPattern and inOperator == kOpContains)
		result = new CDbAllDocIterator(Count());
	else
		result = GetIndexer()->CreateDocIterator(inIndex, inKey, inKeyIsPattern, inOperator);
	return result;
}

void CDatabank::RecalculateDocumentWeights(const std::string& inIndex)
{
	HFile::SafeSaver save(GetWeightFileURL(inIndex));
	
	int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
	auto_ptr<HStreamBase> file(new HFileStream(save.GetURL(), mode));
	
	GetIndexer()->RecalculateDocumentWeights(inIndex, *file.get());

	save.Commit();
}

// DatabankHeader I/O

HStreamBase& operator<<(HStreamBase& inData, SHeader& inStruct)
{
	inStruct.size = sizeof(inStruct);
	
	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	
	data << inStruct.size << inStruct.entries
		 << inStruct.data_offset << inStruct.data_size
		 << inStruct.index_offset << inStruct.index_size
		 << inStruct.info_offset << inStruct.info_size
		 << inStruct.id_offset << inStruct.id_size
		 << inStruct.blast_ix_offset << inStruct.blast_ix_size
		 << inStruct.uuid;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SHeader& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size >> inStruct.entries
		 >> inStruct.data_offset >> inStruct.data_size
		 >> inStruct.index_offset >> inStruct.index_size
		 >> inStruct.info_offset >> inStruct.info_size
		 >> inStruct.id_offset >> inStruct.id_size
		 >> inStruct.blast_ix_offset >> inStruct.blast_ix_size
		 >> inStruct.uuid;
	
	if (inStruct.size != sizeof(inStruct))
	{
		SHeader t = { 0 };
		
		if (inStruct.size > sizeof(SHeader))
			inStruct.size = sizeof(SHeader);
		
		memcpy(&t, &inStruct, inStruct.size);
		inStruct = t;
		
		inData.Seek(offset + inStruct.size, SEEK_SET);
	}
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, SDataHeader& inStruct)
{
	inStruct.size = sizeof(inStruct);

	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	
	data << inStruct.size << inStruct.count;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SDataHeader& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size >> inStruct.count;
	
	if (inStruct.size != sizeof(inStruct))
		inData.Seek(offset + inStruct.size, SEEK_SET);
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, SDataPart& inStruct)
{
	inStruct.size = sizeof(inStruct);

	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	
	data << inStruct.size
		<< inStruct.data_offset << inStruct.data_size
		<< inStruct.table_offset << inStruct.table_size
		<< inStruct.count;
	data.Write(&inStruct.kind, sizeof(inStruct.kind));
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SDataPart& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size
		>> inStruct.data_offset >> inStruct.data_size
		>> inStruct.table_offset >> inStruct.table_size
		>> inStruct.count;
	data.Read(&inStruct.kind, sizeof(inStruct.kind));
	
	if (inStruct.size != sizeof(inStruct))
		inData.Seek(offset + inStruct.size, SEEK_SET);
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, SBlastIndexHeader& inStruct)
{
	inStruct.size = sizeof(inStruct);
	
	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	
	data << inStruct.size
		 << inStruct.data_offset << inStruct.data_size
		 << inStruct.table_offset << inStruct.table_size
		 << inStruct.db_length << inStruct.seq_count
		 << inStruct.count;

	data.Write(&inStruct.kind, sizeof(inStruct.kind));
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SBlastIndexHeader& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size
		 >> inStruct.data_offset >> inStruct.data_size
		 >> inStruct.table_offset >> inStruct.table_size
		 >> inStruct.db_length >> inStruct.seq_count
		 >> inStruct.count;

	data.Read(&inStruct.kind, sizeof(inStruct.kind));
	
	if (inStruct.size != sizeof(inStruct))
	{
		uint32 n = inStruct.size;
		if (n > sizeof(inStruct))
			n = sizeof(inStruct);
		
		SBlastIndexHeader t = { 0 };
		memcpy(&t, &inStruct, n);
		inStruct = t;
		
		inData.Seek(offset + inStruct.size, SEEK_SET);
	}
	
	return inData;
}

// ---------------------------------------------------------------------------
//
// Joined databank
// 

CJoinedDatabank::CJoinedDatabank(vector<CDatabankBase*>& inParts)
	: fParts(new CPartInfo[inParts.size()])
	, fPartCount(inParts.size())
	, fIndices(nil)
	, fIndexCount(0)
	, fCount(0)
{
	memset(fParts, 0, sizeof(CPartInfo) * fPartCount);

	set<string> indexes;
	string code, type;
	uint32 count;

	for (uint32 ix = 0; ix < fPartCount; ++ix)
	{
		fParts[ix].fDb = inParts[ix];
		fParts[ix].fCount = fParts[ix].fDb->Count();
		
		for (uint32 n = 0; n < fParts[ix].fDb->GetIndexCount(); ++n)
		{
			fParts[ix].fDb->GetIndexInfo(n, code, type, count);
			indexes.insert(code);
		}
	}
	
	fIndexCount = indexes.size();
	fIndices = new CJoinedIndexInfo[fIndexCount];
	
	// now check that all indices have at least the same type...
	uint32 ii = 0;
	for (set<string>::iterator i = indexes.begin(); i != indexes.end(); ++i, ++ii)
	{
		fIndices[ii].fCode = *i;
		fIndices[ii].fCount = 0;
		
		for (uint32 j = 0; j < fPartCount; ++j)
		{
			uint32 nr = fParts[j].fDb->GetIndexNr(*i);
			
			if (nr == fParts[j].fDb->GetIndexCount())
				continue;
			
			fParts[j].fDb->GetIndexInfo(nr, code, type, count);
			
			fIndices[ii].fCount += count;
			
			if (fIndices[ii].fType.length() == 0)
				fIndices[ii].fType = type;
			else if (fIndices[ii].fType != type)
				THROW(("cannot create joined databank since the type for index %s differs",
					code.c_str()));
		}
	}
}

CJoinedDatabank::~CJoinedDatabank()
{
	if (fParts != nil)
	{
		for (uint32 ix = 0; ix < fPartCount; ++ix)
			delete fParts[ix].fDb;

		delete[] fParts;
	}
	
	delete[] fIndices;
}

string CJoinedDatabank::GetDocument(uint32 inDocNr)
{
	string result;
	CDatabankBase* db;
	
	if (PartForDoc(inDocNr, db))
		result = db->GetDocument(inDocNr);
	else
		THROW(("Document number(%d) out of range", inDocNr));
	
	return result;
}

string CJoinedDatabank::GetDocumentID(uint32 inDocNr) const
{
	string result;
	CDatabankBase* db;
	
	if (PartForDoc(inDocNr, db))
		result = db->GetDocumentID(inDocNr);

	return result;
}

bool CJoinedDatabank::GetDocumentNr(const string& inDocID, uint32& outDocNr) const
{
	bool found = false;
	outDocNr = 0;
	
	for (uint32 d = 0; d < fPartCount and not found; ++d)
	{
		uint32 nr;
		if (fParts[d].fDb->GetDocumentNr(inDocID, nr))
		{
			outDocNr += nr;
			found = true;
		}
		else
			outDocNr += fParts[d].fCount;
	}
	
	return found;
}

#ifndef NO_BLAST
uint32 CJoinedDatabank::GetBlastDbCount() const
{
	uint32 result = 0;
	for (uint32 p = 0; p < fPartCount; ++p)
		result += fParts[p].fDb->GetBlastDbCount();
	return result;
}

int64 CJoinedDatabank::GetBlastDbLength() const
{
	int64 result = 0;
	for (uint32 p = 0; p < fPartCount; ++p)
		result += fParts[p].fDb->GetBlastDbLength();
	return result;
}

uint32 CJoinedDatabank::CountSequencesForDocument(uint32 inDocNr)
{
	CDatabankBase* db;
	if (not PartForDoc(inDocNr, db))
		THROW(("Doc nr out of range"));
	
	return db->CountSequencesForDocument(inDocNr);
}

void CJoinedDatabank::GetSequence(uint32 inDocNr, uint32 inIndex,
							CSequence& outSequence)
{
	CDatabankBase* db;
	if (not PartForDoc(inDocNr, db))
		THROW(("Doc nr out of range"));
	
	db->GetSequence(inDocNr, inIndex, outSequence);
}
#endif

uint32 CJoinedDatabank::Count() const
{
	uint32 result = 0;
	for (uint32 p = 0; p < fPartCount; ++p)
		result += fParts[p].fCount;
	return result;
}

void CJoinedDatabank::PrintInfo()
{
	for (uint32 p = 0; p < fPartCount; ++p)
		fParts[p].fDb->PrintInfo();
}

long CJoinedDatabank::GetIndexCount()
{
	return fIndexCount;
}

void CJoinedDatabank::GetIndexInfo(uint32 inIndexNr, string& outCode,
							string& outType, uint32& outCount)
{
	if (inIndexNr >= fIndexCount)
		THROW(("Index number out of range"));
	
	outCode = fIndices[inIndexNr].fCode;
	outType = fIndices[inIndexNr].fType;
	outCount = fIndices[inIndexNr].fCount;
}
	
uint32 CJoinedDatabank::CountDocumentsContainingKey(const string& inIndex,
							const string& inKey)
{
	uint32 result = 0;
	for (uint32 p = 0; p < fPartCount; ++p)
		result += fParts[p].fDb->CountDocumentsContainingKey(inIndex, inKey);
	return result;
}

CIteratorBase* CJoinedDatabank::GetIteratorForIndex(const string& inIndex)
{
	vector<CIteratorBase*> iters;
	for (uint32 p = 0; p < fPartCount; ++p)
		iters.push_back(fParts[p].fDb->GetIteratorForIndex(inIndex));
	return new CStrUnionIterator(iters);
}

CIteratorBase* CJoinedDatabank::GetIteratorForIndexAndKey(
	const string& inIndex, const string& inKey)
{
	vector<CIteratorBase*> iters;
	for (uint32 p = 0; p < fPartCount; ++p)
		iters.push_back(fParts[p].fDb->GetIteratorForIndexAndKey(inIndex, inKey));
	return new CStrUnionIterator(iters);
}

CDocIterator* CJoinedDatabank::CreateDocIterator(
	const string& inIndex, const string& inKey, bool inKeyIsPattern, CQueryOperator inOperator)
{
	vector<CDocIterator*> imps;
	uint32 first = 0;
	
	for (uint32 ix = 0; ix < fPartCount; ++ix)
	{
		CDocIterator* imp = fParts[ix].fDb->CreateDocIterator(
			inIndex, inKey, inKeyIsPattern, inOperator);
		
		if (imp != nil)
		{
			if (first > 0)
				imps.push_back(new CDocDeltaIterator(imp, first));
			else
				imps.push_back(imp);
		}
		
		first += fParts[ix].fCount;
	}
	
	return CDocUnionIterator::Create(imps);
}

CDbDocIteratorBase* CJoinedDatabank::GetDocWeightIterator(
	const string& inIndex, const string& inKey)
{
	auto_ptr<CDbJoinedIterator> result(new CDbJoinedIterator());
	
	for (uint32 ix = 0; ix < fPartCount; ++ix)
	{
		CDbDocIteratorBase* imp =
			fParts[ix].fDb->GetDocWeightIterator(inIndex, inKey);
		
		if (imp == nil)
			continue;
		
		result->AddIterator(imp, fParts[ix].fCount);
	}
	
	return result.release();
}

bool CJoinedDatabank::PartForDoc(uint32& ioDocNr, CDatabankBase*& outDb) const
{
	bool result = false;
	
	uint32 ix = 0;
	while (ix < fPartCount and ioDocNr >= fParts[ix].fCount)
	{
		ioDocNr -= fParts[ix].fCount;
		++ix;
	}
	
	if (ix < fPartCount)
	{
		result = true;
		outDb = fParts[ix].fDb;
	}
	
	return result;
}

string CJoinedDatabank::GetVersion() const
{
	string vers;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			vers += '+';
		vers += fParts[p].fDb->GetVersion();
	}
	return vers;
}

string CJoinedDatabank::GetDbName() const
{
	string result;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			result += '+';
		result += fParts[p].fDb->GetDbName();
	}
	return result;
}

// ---------------------------------------------------------------------------
//
//	Updated databank
//

CUpdatedDatabank::CUpdatedDatabank(const HUrl& inFile, CDatabankBase* inOriginal)
	: CDatabank(inFile, false)
	, fOriginal(inOriginal)
{
}

CUpdatedDatabank::~CUpdatedDatabank()
{
	delete fOriginal;
}

string CUpdatedDatabank::GetDocument(uint32 inDocNr)
{
	if (inDocNr >= Count())
		return fOriginal->GetDocument(inDocNr - Count());
	else
		return CDatabank::GetDocument(inDocNr);
}

#ifndef NO_BLAST
void CUpdatedDatabank::GetSequence(uint32 inDocNr, uint32 inIndex,
	CSequence& outSequence)
{
	if (inDocNr >= Count())
		fOriginal->GetSequence(inDocNr - Count(), inIndex, outSequence);
	else
		CDatabank::GetSequence(inDocNr, inIndex, outSequence);
}

uint32 CUpdatedDatabank::CountSequencesForDocument(uint32 inDocNr)
{
	if (inDocNr >= Count())
		return fOriginal->CountSequencesForDocument(inDocNr - Count());
	else
		return CDatabank::CountSequencesForDocument(inDocNr);
}

uint32 CUpdatedDatabank::GetBlastDbCount() const
{
	return fOriginal->GetBlastDbCount() + CDatabank::GetBlastDbCount();
}

int64 CUpdatedDatabank::GetBlastDbLength() const
{
	return fOriginal->GetBlastDbLength() + CDatabank::GetBlastDbLength();
}
#endif

bool CUpdatedDatabank::GetDocumentNr(const string& inDocumentID, uint32& outDocNr) const
{
	bool result = CDatabank::GetDocumentNr(inDocumentID, outDocNr);
	
	if (not result and fOriginal->GetDocumentNr(inDocumentID, outDocNr))
	{
		result = true;
		outDocNr += Count();
	}
	
	return result;
}

string CUpdatedDatabank::GetDocumentID(uint32 inDocNr) const
{
	if (inDocNr >= Count())
		return fOriginal->GetDocumentID(inDocNr - Count());
	else
		return CDatabank::GetDocumentID(inDocNr);
}

template<class T>
class CUpdateIterator : public CDbDocIteratorBase
{
  public:
						CUpdateIterator(CDatabankBase* inDb, CIndex* inOmit,
							T* inOriginal);
	virtual				~CUpdateIterator();
	
	virtual bool		Next(uint32& ioDoc, bool inSkip);
	virtual bool		Next(uint32& ioDoc, uint8& outRank, bool inSkip);

	virtual uint32		Count() const		{ return fOriginal->Count(); }
	virtual uint32		Read() const		{ return fOriginal->Read(); }

  protected:	
	CDatabankBase*		fDb;
	CIndex*				fOmit;
	T*					fOriginal;
};

template<class T>
CUpdateIterator<T>::CUpdateIterator(CDatabankBase* inDb, CIndex* inOmit, T* inOriginal)
	: fDb(inDb)
	, fOmit(inOmit)
	, fOriginal(inOriginal)
{
}

template<class T>
CUpdateIterator<T>::~CUpdateIterator()
{
	delete fOriginal;
	delete fOmit;
}

template<class T>
bool CUpdateIterator<T>::Next(uint32& outDocNr, bool inSkip)
{
	THROW(("runtime error"));
	return false;
}

template<class T>
bool CUpdateIterator<T>::Next(uint32& outDocNr, uint8& outRank, bool inSkip)
{
	THROW(("runtime error"));
	return false;
}

template<>
bool CUpdateIterator<CDocIterator>::Next(uint32& outDocNr, bool inSkip)
{
	bool result = false;
	while (not result and fOriginal != nil and fOriginal->Next(outDocNr, inSkip))
	{
		string id = fDb->GetDocumentID(outDocNr);
		
		uint32 v;
		if (not fOmit->GetValue(id, v))
			result = true;
	}
	return result;
}

template<>
bool CUpdateIterator<CDbDocIteratorBase>::Next(uint32& outDocNr, uint8& outRank, bool inSkip)
{
	bool result = false;
	while (not result and fOriginal != nil and fOriginal->Next(outDocNr, outRank, inSkip))
	{
		string id = fDb->GetDocumentID(outDocNr);
		
		uint32 v;
		if (not fOmit->GetValue(id, v))
			result = true;
	}
	return result;
}

//bool CUpdateIterator::Next(uint32& outDocNr, bool inSkip)
//{
//	uint8 r;
//	return Next(outDocNr, r, inSkip);
//}

CDocIterator* CUpdatedDatabank::CreateDocIterator(const string& inIndex,
	const string& inKey, bool inKeyIsPattern, CQueryOperator inOperator)
{
	CIndex* omit = GetIndexer()->GetIndex("id");
	if (omit == nil)
		THROW(("Update databank does not contain an id index"));

	CDocIterator* a = CDatabank::CreateDocIterator(inIndex, inKey, inKeyIsPattern, inOperator);
	CDocIterator* b = new CDocDeltaIterator(new CUpdateIterator<CDocIterator>(fOriginal, omit,
		fOriginal->CreateDocIterator(inIndex, inKey, inKeyIsPattern, inOperator)), Count());
	
	return CDocUnionIterator::Create(a, b);
}

uint32 CUpdatedDatabank::Count() const
{
	return CDatabank::Count() + fOriginal->Count();
}

void CUpdatedDatabank::PrintInfo()
{
	cout << "Original databank: " << endl;
	fOriginal->PrintInfo();

	cout << "Update databank: " << endl;
	CDatabank::PrintInfo();
}

uint32 CUpdatedDatabank::CountDocumentsContainingKey(const string& inIndex, const string& inKey)
{
	return CDatabank::CountDocumentsContainingKey(inIndex, inKey) +
		fOriginal->CountDocumentsContainingKey(inIndex, inKey);
}

string CUpdatedDatabank::GetVersion() const
{
	return fOriginal->GetVersion() + '|' + CDatabank::GetVersion();
}

CDbDocIteratorBase* CUpdatedDatabank::GetDocWeightIterator(
	const std::string& inIndex, const std::string& inKey)
{
//	THROW(("not supported yet, sorry"));

	CIndex* omit = GetIndexer()->GetIndex("id");
	if (omit == nil)
		THROW(("Update databank does not contain an id index"));

	CDbDocIteratorBase* a = CDatabank::GetDocWeightIterator(inIndex, inKey);
	CDbDocIteratorBase* b = new CUpdateIterator<CDbDocIteratorBase>(fOriginal, omit,
		fOriginal->GetDocWeightIterator(inIndex, inKey));
	
	return new CMergedDbDocIterator(a, 0, fOriginal->Count(), b, Count(), Count());
}

string	CUpdatedDatabank::GetDbName() const
{
	return fOriginal->GetDbName() + '_' + CDatabank::GetDbName();
}

void CUpdatedDatabank::RecalculateDocumentWeights(const std::string& inIndex)
{
	CDatabankBase::RecalculateDocumentWeights(inIndex);
}

