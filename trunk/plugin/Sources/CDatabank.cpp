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
#include "HMutex.h"
#include <iostream>
#include <deque>
#include <set>
#include <fcntl.h>
#include "HUtils.h"
#include "HByteSwap.h"
#include "HStream.h"
#include "HUtils.h"
#include "zlib.h"
#include "uuid/uuid.h"

#include "CDatabank.h"
#include "CCompress.h"
#include "CDecompress.h"
#include "CIndexer.h"
#include "CIndex.h"
#include "CIterator.h"
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
	kMetaDataSig = FOUR_CHAR_INLINE('meta'),
	kPartSig = FOUR_CHAR_INLINE('part'),
	kBlastIndexSignature = FOUR_CHAR_INLINE('blst'),
	
	kStopWordKind = FOUR_CHAR_INLINE('stop');

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
	int64		omit_vector_offset;		// a bit vector for records that are invalid
};

const uint32
	kSHeaderSizeV0 = 2 * sizeof(uint32) + 11 * sizeof(int64),
	kSHeaderSizeV1 = 2 * sizeof(uint32) + 11 * sizeof(int64) + sizeof(uuid_t),
	kSHeaderSizeV2 = 2 * sizeof(uint32) + 11 * sizeof(int64) + sizeof(uuid_t) + sizeof(int64),
	
	kSHeaderSize = kSHeaderSizeV2;

struct SDataHeader
{
	uint32			sig;
	uint32			size;
	uint32			count;
	uint32			meta_data_count;
};

struct SMetaData
{
	uint32			sig;
	uint32			size;
	char			name[16];
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
	int64			raw_data_size;	// size of raw document data
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
HStreamBase& operator<<(HStreamBase& inData, SMetaData& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SMetaData& inStruct);
HStreamBase& operator<<(HStreamBase& inData, SDataPart& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SDataPart& inStruct);
HStreamBase& operator<<(HStreamBase& inData, SBlastIndexHeader& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SBlastIndexHeader& inStruct);

/*
	Base classes
*/

CDatabankBase::CDatabankBase()
	: fDictionary(nil)
	, fLock(new HMutex)
{
}

CDatabankBase::~CDatabankBase()
{
	delete fLock;
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

bool CDatabankBase::IsValidDocumentNr(uint32 inDocNr) const
{
	return true;
}

string CDatabankBase::GetDocument(const string& inDocumentID)
{
	return GetDocument(GetDocumentNr(inDocumentID));
}

string CDatabankBase::GetMetaData(const string& inDocumentID, const string& inName)
{
	return GetMetaData(GetDocumentNr(inDocumentID), inName);
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

CDocWeightArray CDatabankBase::GetDocWeights(const std::string& inIndex)
{
	THROW(("GetDocWeights unsupported yet"));
//	HUrl url = GetWeightFileURL(inIndex);
//	
//	if (not HFile::Exists(url))
//		RecalculateDocumentWeights(inIndex);
//	
//	CDocWeightArray arr(url, Count());
//	return arr;
}

uint32 CDatabankBase::GetMaxWeight() const
{
	THROW(("GetMaxWeight unsupported"));
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
	StMutex lock(*fLock);
	
	if (fDictionary == nil)
		fDictionary = new CDictionary(*this);

	return fDictionary->SuggestCorrection(inKey);
}

void CDatabankBase::GetStopWords(std::set<std::string>& outStopWords) const
{
}

/*
	Implementations
*/

CDatabank::CDatabank(const HUrl& inPath, const vector<string>& inMetaDataFields,
		const string& inName, const string& inVersion, const string& inURL, const string& inScriptName,
		const string& inSection)
	: fPath(inPath)
	, fModificationTime(0)
	, fDataFile(NULL)
	, fCompressor(NULL)
	, fIndexer(NULL)
	, fReadOnly(false)
	, fInfoContainer(nil)
	, fIdTable(nil)
#ifndef NO_BLAST
	, fBlastIndex(nil)
#endif
	, fHeader(new SHeader)
	, fDataHeader(new SDataHeader)
	, fMetaData(nil)
	, fParts(nil)
#ifndef NO_BLAST
	, fBlast(nil)
#endif
	, fOmitVector(nil)
{
	memset(fHeader, 0, sizeof(SHeader));
	memset(fDataHeader, 0, sizeof(SDataHeader));
	
	int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
	
//	fDataFile = new HBufferedFileStream(fPath, mode);
	fDataFile = new HFileStream(fPath, mode);
	
	fHeader->sig = kHeaderSig;
	
	// generate a uuid based on time and MAC address
	// that way it is easier to track where DB's come from.
	uuid_generate_time(fHeader->uuid);
	
	*fDataFile << *fHeader;

	fHeader->data_offset = fDataFile->Tell();
	
	fDataHeader->sig = kDataSig;
	fDataHeader->count = 1;
	fDataHeader->meta_data_count = inMetaDataFields.size();
	
	*fDataFile << *fDataHeader;
	
	if (fDataHeader->meta_data_count > 0)
	{
		fMetaData = new CMetaData[fDataHeader->meta_data_count];
		
		HAutoBuf<SMetaData> mb(new SMetaData[fDataHeader->meta_data_count]);
		memset(mb.get(), 0, sizeof(SMetaData) * fDataHeader->meta_data_count);
		
		uint32 i = 0;
		for (vector<string>::const_iterator md = inMetaDataFields.begin(); md != inMetaDataFields.end(); ++md, ++i)
		{
			strcpy(mb[i].name, md->c_str());
			*fDataFile << mb[i];
			
			strcpy(fMetaData[i].name, md->c_str());
		}
	}
	
	fParts = new SDataPart[1];
	memset(fParts, 0, sizeof(SDataPart));
	fParts[0].sig = kPartSig;
	*fDataFile << fParts[0];
	
	fCompressor = new CCompressor(*fDataFile, inPath);
	fIndexer = new CIndexer(inPath);

	fInfoContainer = new CDbInfo;
	
	fInfoContainer->Add('name', inName);
	fInfoContainer->Add('scrp', inScriptName);
	fInfoContainer->Add('iurl', inURL);
	fInfoContainer->Add('sect', inSection);

	if (inVersion.length() > 0)
		fInfoContainer->Add('vers', inVersion);
}

CDatabank::CDatabank(const HUrl& inUrl)
	: fPath(inUrl)
	, fModificationTime(0)
	, fDataFile(NULL)
	, fCompressor(NULL)
	, fIndexer(NULL)
	, fReadOnly(true)
	, fInfoContainer(nil)
	, fIdTable(nil)
#ifndef NO_BLAST
	, fBlastIndex(nil)
#endif
	, fHeader(new SHeader)
	, fDataHeader(new SDataHeader)
	, fMetaData(nil)
	, fParts(nil)
#ifndef NO_BLAST
	, fBlast(nil)
#endif
	, fOmitVector(nil)
{
	HFile::GetModificationTime(fPath, fModificationTime);
	
	assert(sizeof(SHeader) == kSHeaderSize);
	
	memset(fHeader, 0, sizeof(SHeader));
	memset(fDataHeader, 0, sizeof(SDataHeader));
	
	int mode = O_RDONLY | O_BINARY;
	
	fDataFile = new HBufferedFileStream(fPath, mode);
	
	*fDataFile >> *fHeader;

	if (fHeader->sig != kHeaderSig)
		THROW(("Not a mrs data file"));
	
	if (fHeader->data_offset == 0 or fHeader->data_size == 0 or
		fHeader->index_offset == 0 or fHeader->index_size == 0)
	{
		THROW(("Invalid mrs data file"));
	}
	
	fIndexer = new CIndexer(*fDataFile, fHeader->index_offset, fHeader->index_size);
	
	if (fHeader->info_size > 0)
	{
		HStreamView s(*fDataFile, fHeader->info_offset, fHeader->info_size);
		fInfoContainer = new CDbInfo(s);
	}

	if (fHeader->id_size > 0)
		fIdTable = new CIdTable(*fDataFile, fHeader->id_offset, fHeader->id_size);

	fDataFile->Seek(fHeader->data_offset, SEEK_SET);
	(*fDataFile) >> *fDataHeader;
	
	if (fDataHeader->count == 0)
		THROW(("Invalid mrs data file"));
	
	if (fDataHeader->meta_data_count > 0)
	{
		fMetaData = new CMetaData[fDataHeader->meta_data_count];
		
		for (uint32 i = 0; i < fDataHeader->meta_data_count; ++i)
		{
			SMetaData md;
			*fDataFile >> md;
			
			strcpy(fMetaData[i].name, md.name);
		}
	}
	
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
	delete[] fOmitVector;
}

string CDatabank::GetDbName() const
{
	return basename(fPath.GetFileName());
}

void CDatabank::Finish(bool inCreateAllTextIndex, bool inCreateUpdateDatabank)
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

	fDataFile->Seek(fHeader->data_offset + sizeof(SDataHeader) + fDataHeader->meta_data_count * sizeof(SMetaData), SEEK_SET);
	*fDataFile << partInfo;

	delete fCompressor;
	fCompressor = NULL;
	
	fDataFile->Seek(0, SEEK_END);
	
	fIndexer->CreateIndex(*fDataFile, fHeader->index_offset,
		fHeader->index_size, inCreateAllTextIndex);

	auto_ptr<CIndex> idIndex(fIndexer->GetIndex("id"));
	if (idIndex.get() != nil)
	{
		if (VERBOSE >= 1)
		{
			cout << "Creating ID table... ";
			cout.flush();
		}

		// create an omit vector here if needed
		
		uint32 omitVectorSize = fHeader->entries >> 3;
		if (inCreateUpdateDatabank)
		{
			delete[] fOmitVector;
			if (fHeader->entries & 0x07)
				omitVectorSize += 1;
			
			fOmitVector = new uint8[omitVectorSize];
			memset(fOmitVector, 0, omitVectorSize);
		}
	
		fDataFile->Seek(0, SEEK_END);
		fHeader->id_offset = fDataFile->Tell();
		CIdTable::Create(*fDataFile, *idIndex.get(), fHeader->entries, fOmitVector);
		fHeader->id_size = fDataFile->Tell() - fHeader->id_offset;
		
		if (inCreateUpdateDatabank)
		{
			fHeader->omit_vector_offset = fDataFile->Tell();
			fDataFile->Write(fOmitVector, omitVectorSize);
		}

		if (VERBOSE >= 1)
			cout << "done" << endl;

		idIndex.release();
	}
	else if (VERBOSE >= 1)
		cout << "No ID table created since there is no id index" << endl;

	fIndexer->FixupDocWeights();

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

void CDatabank::Merge(vector<CDatabank*>& inParts, bool inCopyData)
{
	if (fReadOnly)
	{
		delete fDataFile;
		int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
//		fDataFile = new HBufferedFileStream(fPath, mode);
		fDataFile = new HFileStream(fPath, mode);
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

	fDataHeader->meta_data_count = inParts.front()->fDataHeader->meta_data_count;

	if (fParts != nil)
		delete[] fParts;

	fParts = new SDataPart[fDataHeader->count];
	memset(fParts, 0, sizeof(SDataPart) * fDataHeader->count);

		// reserve space
	(*fDataFile) << *fDataHeader;

	if (fDataHeader->meta_data_count > 0)
	{
		CDatabank* p1 = inParts[0];
		
		for (uint32 i = 0; i < p1->fDataHeader->meta_data_count; ++i)
		{
			SMetaData md = {};
			md.sig = kMetaDataSig;
			md.size = sizeof(SMetaData);
			strcpy(md.name, p1->fMetaData[i].name);
			
			*fDataFile << md;
			
			for (uint32 j = 1; j < inParts.size(); ++j)
			{
				CDatabank* p2 = inParts[j];

				if (p1->fDataHeader->meta_data_count != p2->fDataHeader->meta_data_count)
					THROW(("All parts should have the same meta data fields"));
				
				if (strcmp(p1->fMetaData[i].name, p2->fMetaData[i].name) != 0)
					THROW(("All parts should have the same meta data fields"));
			}
		}
	}

	int64 partOffset = fDataFile->Tell();

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
			
			if (inCopyData)
			{
				(*d)->GetDecompressor(ix)->CopyData(*fDataFile, fParts[part].kind,
					fParts[part].data_offset, fParts[part].data_size,
					fParts[part].table_offset, fParts[part].table_size);
			}
			else
			{
				(*d)->GetDecompressor(ix)->LinkData(
					(*d)->GetDataUrl().GetFileName(), (*d)->GetUUID(),
					*fDataFile, fParts[part].kind,
					fParts[part].data_offset, fParts[part].data_size,
					fParts[part].table_offset, fParts[part].table_size);
			}

			fParts[part].count = (*d)->fParts[ix].count;
			fParts[part].raw_data_size = (*d)->fParts[ix].raw_data_size;
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
	
	fDataFile->Seek(partOffset, SEEK_SET);
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

	auto_ptr<CIndex> idIndex(index.GetIndex("id"));
	if (idIndex.get() != nil)
	{
		if (VERBOSE >= 1)
		{
			cout << "Creating ID table... ";
			cout.flush();
		}
		
		fDataFile->Seek(0, SEEK_END);
		fHeader->id_offset = fDataFile->Tell();
		
		// since we're merging we create an omit vector here
		delete[] fOmitVector;
		uint32 omitVectorSize = fHeader->entries >> 3;
		if (fHeader->entries & 0x07)
			omitVectorSize += 1;
		
		fOmitVector = new uint8[omitVectorSize];
		memset(fOmitVector, 0, omitVectorSize);
		
		CIdTable::Create(*fDataFile, *idIndex.get(), fHeader->entries, fOmitVector);

		fDataFile->Seek(0, SEEK_END);
		fHeader->id_size = fDataFile->Tell() - fHeader->id_offset;
		
		fHeader->omit_vector_offset = fDataFile->Tell();
		fDataFile->Write(fOmitVector, omitVectorSize);

		if (VERBOSE >= 1)
			cout << "done" << endl;
		
		idIndex.release();
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
	
	assert(fInfoContainer);

	for (d = inParts.begin(); d != inParts.end(); ++d)
	{
		if ((*d)->fInfoContainer == nil)
			continue;
		
		if (VERBOSE)
		{
			cout << "Copying info from " << (*d)->fPath.GetFileName() << " ";
			cout.flush();
		}

		uint32 cookie = 0;
		string s;
		uint32 kind;
		
		while ((*d)->fInfoContainer->Next(cookie, s, kind))
		{
			if (kind != 'scrp' and kind != 'name' and kind != 'iurl' and kind != 'sect')
				fInfoContainer->Add(kind, s);
		}

		if (VERBOSE)
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
	
	if (fMetaData != nil)
		return GetDecompressor(p - fDataParts.begin())
			->GetField(inDocNr, fDataHeader->meta_data_count);	// field after last meta field is document
	else
		return GetDecompressor(p - fDataParts.begin())
			->GetDocument(inDocNr);
}

string CDatabank::GetMetaData(uint32 inDocNr, const string& inName)
{
	if (fMetaData == nil)
		THROW(("This db does not contain meta data"));
	
	CPartList::iterator p = fDataParts.begin();
	while (p != fDataParts.end() and inDocNr >= (*p).fCount)
	{
		inDocNr -= (*p).fCount;
		++p;
	}
	
	if (p == fDataParts.end())
		THROW(("Doc number out of range"));
	
	uint32 ix = 0;
	while (ix < fDataHeader->meta_data_count)
	{
		if (inName == fMetaData[ix].name)
			break;
		++ix;
	}
	
	if (ix == fDataHeader->meta_data_count)
		THROW(("Meta data field not found"));
	
	return GetDecompressor(p - fDataParts.begin())
		->GetField(inDocNr, ix);
}

vector<string> CDatabank::GetMetaDataFields() const
{
	vector<string> result;
	
	if (fMetaData)
	{
		for (uint32 ix = 0; ix < fDataHeader->meta_data_count; ++ix)
			result.push_back(fMetaData[ix].name);
	}
	
	return result;
}

bool CDatabank::GetDocumentNr(const string& inDocID, uint32& outDocNr) const
{
	bool result = true;

	// our ID table inserts document ID's with the pattern '#docnr'
	// make sure we recognize those	
	if (not GetIndexer()->GetDocumentNr(inDocID, outDocNr) and
		inDocID.length() > 1 and
		inDocID[0] == '#')
	{
		const char* p = inDocID.c_str() + 1;
		
		outDocNr = 0;
	
		// parse number		
		while (isdigit(*p))
		{
			outDocNr = outDocNr * 10 + (*p - '0');
			++p;
		}
		
		if (*p != 0)
			result = false;
	}
	
	return result;
}

bool CDatabank::IsValidDocumentNr(uint32 inDocNr) const
{
	bool result = true;
	
	if (fHeader->omit_vector_offset > 0)
	{
		StMutex lock(*fLock);

		if (fOmitVector == nil)
		{
			uint32 omitVectorSize = fHeader->entries >> 3;
			if (fHeader->entries & 0x07)
				omitVectorSize += 1;
			
			fOmitVector = new uint8[omitVectorSize];
			fDataFile->PRead(fOmitVector, omitVectorSize, fHeader->omit_vector_offset);
		}
		
		uint32 byte = inDocNr >> 3;
		uint32 bit = inDocNr & 0x07;
		
		result = (fOmitVector[byte] & (1 << bit)) == 0;
	}
	
	return result;
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
			new CDecompressor(fPath, *fDataFile, fParts[inPartNr].kind,
				fParts[inPartNr].data_offset, fParts[inPartNr].data_size,
				fParts[inPartNr].table_offset, fParts[inPartNr].table_size,
				fDataHeader->meta_data_count);
	}
	
	return fDataParts[inPartNr].fDecompressor;
}

uint32 CDatabank::Count() const
{
	return fHeader->entries;
}

int64 CDatabank::GetRawDataSize() const
{
	int64 result = 0;
	for (uint32 i = 0; i < fDataHeader->count; ++i)
		result += fParts[i].raw_data_size;
	return result;
}

bool CDatabank::GetInfo(uint32 inKind, uint32 inIndex, string& outText) const
{
	bool result = false;
	
	if (fInfoContainer != nil)
	{
		uint32 cookie = 0, k;
		
		while (inIndex-- > 0 and fInfoContainer->Next(cookie, outText, k, inKind))
			;
		
		if (fInfoContainer->Next(cookie, outText, k, inKind))
			result = true;
	}
	
	return result;
}

string CDatabank::GetVersion() const
{
	uint32 ix = 0;
	string s, l, vers;
	
	while (GetInfo('vers', ix++, s))
	{
		if (s != l)		// avoid adding a duplicate version string
		{
			if (vers.length())
				vers += '\t';
			vers += s;
			
			l = s;
		}
	}
	
	return vers;
}

string CDatabank::GetUUID() const
{
	char suuid[40];
	uuid_unparse(fHeader->uuid, suuid);
	return suuid;
}

string CDatabank::GetName() const
{
	string result;
	
	if (not GetInfo('name', 0, result))
		result = GetDbName();

	return result;
}

string CDatabank::GetInfoURL() const
{
	string result;
	
	if (not GetInfo('iurl', 0, result))
		result = kEmptyString;

	return result;
}

string CDatabank::GetScriptName() const
{
	string result;
	
	if (not GetInfo('scrp', 0, result))
		result = kEmptyString;

	return result;
}

string CDatabank::GetSection() const
{
	string result;
	
	if (not GetInfo('sect', 0, result))
		result = kEmptyString;

	return result;
}

bool CDatabank::IsUpToDate() const
{
	int64 mtime;
	
	return
		HFile::GetModificationTime(fPath, mtime) == 0 and mtime == fModificationTime;
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

CDocWeightArray CDatabank::GetDocWeights(const std::string& inIndex)
{
	return GetIndexer()->GetDocWeights(inIndex);
}

uint32 CDatabank::GetMaxWeight() const
{
	return GetIndexer()->GetMaxWeight();
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

	char suuid[40];
	uuid_unparse(fHeader->uuid, suuid);

	int64 rawDataSize = 0;
	for (uint32 ix = 0; ix < fDataHeader->count; ++ix)
		rawDataSize += fParts[ix].raw_data_size;

	cout << "Header:" << endl;
	cout << "  signature:     " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
	cout << "  uuid:          " << suuid << endl;
	
	cout << "  name:          " << GetName() << endl;
	cout << "  version:       " << GetVersion() << endl;
	cout << "  url:           " << GetInfoURL() << endl;
	cout << "  script:        " << GetScriptName() << endl;
	cout << "  section:       " << GetSection() << endl;
	
	cout << "  size:          " << fHeader->size << endl;
	cout << "  entries:       " << fHeader->entries << endl;
	cout << "  data offset:   " << fHeader->data_offset << endl;
	cout << "  data size:     " << fHeader->data_size << endl;

	if (rawDataSize != 0)
		cout << "  raw data size: " << rawDataSize << endl;
		
	cout << "  index offset:  " << fHeader->index_offset << endl;
	cout << "  index size:    " << fHeader->index_size << endl;
	cout << "  info offset:   " << fHeader->info_offset << endl;
	cout << "  info size:     " << fHeader->info_size << endl;
	cout << "  id offset:     " << fHeader->id_offset << endl;
	cout << "  id size:       " << fHeader->id_size << endl;
#ifndef NO_BLAST
	cout << "  blast offset:  " << fHeader->blast_ix_offset << endl;
	cout << "  blast size:    " << fHeader->blast_ix_size << endl;
#endif
	cout << "  omitvec offset:" << fHeader->omit_vector_offset << endl;
	cout << endl;
	
	sig = reinterpret_cast<const char*>(&fDataHeader->sig);
	
	cout << "Data Header:" << endl;
	cout << "  signature:    " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
	cout << "  size:         " << fDataHeader->size << endl;
	cout << "  count:        " << fDataHeader->count << endl;

	if (fMetaData != nil)
	{
		cout << "  meta data:    " << fDataHeader->meta_data_count << " { ";

		for (uint32 i = 0; i < fDataHeader->meta_data_count; ++i)
		{
			cout << fMetaData[i].name;
			if (i < fDataHeader->meta_data_count - 1)
				cout << ", ";
		}

		cout << " }" << endl;
	}
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
		
		if (p.raw_data_size != 0)
			cout << "  raw data:     " << p.raw_data_size << endl;

		sig = reinterpret_cast<const char*>(&p.kind);
		cout << "  compression:  " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
		cout << endl;
	}
	
	GetIndexer()->PrintInfo();
	
	if (fIdTable != nil)
		fIdTable->PrintInfo();
	
#ifndef NO_BLAST
	if (fBlast != nil)
	{
		
		
	}
#endif
}

void CDatabank::SetStopWords(const vector<string>& inStopWords)
{
	fIndexer->SetStopWords(inStopWords);
	
	if (fInfoContainer == nil)
		fInfoContainer = new CDbInfo;

	for (vector<string>::const_iterator s = inStopWords.begin(); s != inStopWords.end(); ++s)
		fInfoContainer->Add(kStopWordKind, *s);
}

void CDatabank::GetStopWords(set<string>& outStopWords) const
{
	if (fInfoContainer != nil)
	{
		uint32 cookie = 0, kind;
		string s;
		
		while (fInfoContainer->Next(cookie, s, kind, kStopWordKind))
			outStopWords.insert(s);
	}
}

void CDatabank::Store(const string& inDocument)
{
	if (fMetaData != nil)
	{
		vector<pair<const char*,uint32> > dv;
		
		for (uint32 i = 0; i < fDataHeader->meta_data_count; ++i)
			dv.push_back(make_pair(fMetaData[i].data.c_str(), fMetaData[i].data.length()));

		dv.push_back(make_pair(inDocument.c_str(), inDocument.length()));

		fCompressor->AddData(dv);

		for (uint32 i = 0; i < fDataHeader->meta_data_count; ++i)
			fMetaData[i].data.clear();
	}
	else
		fCompressor->AddDocument(inDocument.c_str(), inDocument.length());

	++fHeader->entries;
	fParts[0].raw_data_size += inDocument.length();
}

void CDatabank::StoreMetaData(const std::string& inFieldName, const std::string& inData)
{
	uint32 i;
	for (i = 0; i < fDataHeader->meta_data_count; ++i)
	{
		if (inFieldName == fMetaData[i].name)
		{
			fMetaData[i].data += inData;
			break;
		}
	}
	
	if (i == fDataHeader->meta_data_count)
		THROW(("Meta data field %s not defined in the MDatabank::Create call", inFieldName.c_str()));
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
		
		if ((fIndexer->Count() % 60000) == 0)
			cout << ' ' << fIndexer->Count() << endl;
		else
			cout.flush();
	}
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

//void CDatabank::RecalculateDocumentWeights(const std::string& inIndex)
//{
//	HFile::SafeSaver save(GetWeightFileURL(inIndex));
//	
//	int mode = O_RDWR | O_CREAT | O_BINARY | O_TRUNC;
//	auto_ptr<HStreamBase> file(new HFileStream(save.GetURL(), mode));
//	
//	GetIndexer()->RecalculateDocumentWeights(inIndex, *file.get());
//
//	save.Commit();
//}

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
		 << inStruct.blast_ix_offset << inStruct.blast_ix_size;
	
	data.Write(inStruct.uuid, sizeof(inStruct.uuid));
	
	data << inStruct.omit_vector_offset;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SHeader& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size >> inStruct.entries
		 >> inStruct.data_offset >> inStruct.data_size
		 >> inStruct.index_offset >> inStruct.index_size
		 >> inStruct.info_offset >> inStruct.info_size
		 >> inStruct.id_offset >> inStruct.id_size
		 >> inStruct.blast_ix_offset >> inStruct.blast_ix_size;
	
	if (inStruct.size >= kSHeaderSizeV1)
		data.Read(inStruct.uuid, sizeof(inStruct.uuid));
	
	if (inStruct.size >= kSHeaderSizeV2)
		data >> inStruct.omit_vector_offset;

	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, SDataHeader& inStruct)
{
	inStruct.size = sizeof(inStruct);

	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	
	data << inStruct.size << inStruct.count << inStruct.meta_data_count;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SDataHeader& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	
	data >> inStruct.size >> inStruct.count;
	
	if (inStruct.size >= sizeof(inStruct))
		data >> inStruct.meta_data_count;
	
	if (inStruct.size != sizeof(inStruct))
		inData.Seek(offset + inStruct.size, SEEK_SET);
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, SMetaData& inStruct)
{
	inStruct.size = sizeof(inStruct);

	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	data << inStruct.size;
	data.Write(inStruct.name, sizeof(inStruct.name));
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SMetaData& inStruct)
{
	int64 offset = inData.Tell();
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	data >> inStruct.size;
	data.Read(inStruct.name, sizeof(inStruct.name));
	
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
	data << inStruct.raw_data_size;
	
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
	
	if (inStruct.size >= sizeof(inStruct))
		data >> inStruct.raw_data_size;
	else
		inStruct.raw_data_size = 0;
	
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
	uint32 maxWeight = 0;

	for (uint32 ix = 0; ix < fPartCount; ++ix)
	{
		fParts[ix].fDb = inParts[ix];
		fParts[ix].fCount = fParts[ix].fDb->Count();
		
		if (ix == 0)
			maxWeight = fParts[0].fDb->GetMaxWeight();
		else if (maxWeight != fParts[ix].fDb->GetMaxWeight())
			THROW(("weight bit count is not equal for all parts"));
		
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

string CJoinedDatabank::GetMetaData(uint32 inDocNr, const string& inName)
{
	string result;
	CDatabankBase* db;
	
	if (PartForDoc(inDocNr, db))
		result = db->GetMetaData(inDocNr, inName);
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

bool CJoinedDatabank::IsValidDocumentNr(uint32 inDocNr) const
{
	bool result = false;
	CDatabankBase* db;
	
	if (PartForDoc(inDocNr, db))
		result = db->IsValidDocumentNr(inDocNr);

	return result;
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

int64 CJoinedDatabank::GetRawDataSize() const
{
	int64 result = 0;
	for (uint32 p = 0; p < fPartCount; ++p)
		result += fParts[p].fDb->GetRawDataSize();
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

CDocWeightArray CJoinedDatabank::GetDocWeights(const std::string& inIndex)
{
	CDocWeightArray dw = fParts[0].fDb->GetDocWeights(inIndex);
	
	for (uint32 ix = 1; ix < fPartCount; ++ix)
		dw = CDocWeightArray(dw, fParts[ix].fDb->GetDocWeights(inIndex));
	
	return dw;
}

uint32 CJoinedDatabank::GetMaxWeight() const
{
	return fParts[0].fDb->GetMaxWeight();
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
			vers += " + ";
		vers += fParts[p].fDb->GetVersion();
	}
	return vers;
}

string CJoinedDatabank::GetUUID() const
{
	string uuid;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			uuid += " + ";
		uuid += fParts[p].fDb->GetUUID();
	}
	return uuid;
}

string CJoinedDatabank::GetName() const
{
	string result;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			result += " + ";
		result += fParts[p].fDb->GetName();
	}
	return result;
}

string CJoinedDatabank::GetInfoURL() const
{
	string result;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			result += " + ";
		result += fParts[p].fDb->GetInfoURL();
	}
	return result;
}

string CJoinedDatabank::GetScriptName() const
{
	string result;
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		if (p > 0)
			result += " + ";
		result += fParts[p].fDb->GetScriptName();
	}
	return result;
}

string CJoinedDatabank::GetSection() const
{
	return fParts[0].fDb->GetSection();
}

bool CJoinedDatabank::IsUpToDate() const
{
	bool result = true;
	
	for (uint32 p = 0; result and p < fPartCount; ++p)
		result = fParts[p].fDb->IsUpToDate();
	
	return result;
}

vector<string> CJoinedDatabank::GetMetaDataFields() const
{
	set<string> fields;
	
	for (uint32 p = 0; p < fPartCount; ++p)
	{
		vector<string> n(fParts[p].fDb->GetMetaDataFields());
		for (vector<string>::iterator m = n.begin(); m != n.end(); ++m)
			fields.insert(*m);
	}
	
	vector<string> result;
	copy(fields.begin(), fields.end(), back_insert_iterator<vector<string> >(result));
	return result;
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

