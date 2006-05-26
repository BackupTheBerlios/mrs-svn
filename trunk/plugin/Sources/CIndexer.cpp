/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 18:35:56
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

#include <list>
#include <set>
#include <iostream>
#include <limits>
#include <ctime>
#include <sstream>
#include <tr1/unordered_set>
#include <boost/functional/hash/hash.hpp>
#include "HFile.h"
#include "HUtils.h"
#include "HError.h"

#include "CIndex.h"
#include "CIterator.h"
#include "CIndexer.h"
#include "CDecompress.h"
#include "CTokenizer.h"
#include "HStream.h"
#include "CBitStream.h"
#include "CLexicon.h"
#include "CUtils.h"
#include "CDocWeightArray.h"

using namespace std;

// Global, settable from the outside

unsigned int WEIGHT_BIT_COUNT = 5;

const char kAllTextIndexName[] = "__ALL_TEXT__";

/*
	Data structures on disk
*/

const uint32
	kIndexSig = FOUR_CHAR_INLINE('indx'),
	kIndexPartSig = FOUR_CHAR_INLINE('indp'),
	kIndexPartSigV2 = FOUR_CHAR_INLINE('Indp'),
	kMaxIndexNr = 31;

struct SIndexHeader
{
	uint32			sig;
	uint32			size;
	int64			entries;
	uint32			count;
	uint32			weight_bit_count;
};

const uint32
	kSIndexHeaderSize = 4 * sizeof(uint32) + sizeof(int64);

struct SIndexPart
{
	uint32			sig;
	uint32			size;
	char			name[16];
	uint32			kind;
	int64			bits_offset;
	int64			tree_offset;
	uint32			root;
	uint32			entries;
	int64			weight_offset;
	
	// members that are implictly part of the above
	bool			large_offsets;
};

const uint32
	kSIndexPartSize = 5 * sizeof(uint32) + 3 * sizeof(int64) + 16;

HStreamBase& operator<<(HStreamBase& inData, const SIndexHeader& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SIndexHeader& inStruct);
HStreamBase& operator<<(HStreamBase& inData, const SIndexPart& inStruct);
HStreamBase& operator>>(HStreamBase& inData, SIndexPart& inStruct);

HStreamBase& operator<<(HStreamBase& inData, const SIndexHeader& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	data << kSIndexHeaderSize << inStruct.entries
		 << inStruct.count << inStruct.weight_bit_count;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SIndexHeader& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	data >> inStruct.size >> inStruct.entries >> inStruct.count
		 >> inStruct.weight_bit_count;
	
	if (inStruct.size != kSIndexHeaderSize)
	{
		SIndexHeader t = {};
		memcpy(&t, &inStruct, min(inStruct.size, kSIndexHeaderSize));
		
		inStruct = t;
		
		if (inStruct.weight_bit_count == 0)
			inStruct.weight_bit_count = 6;
		
		inData.Seek(kSIndexHeaderSize - inStruct.size, SEEK_CUR);
	}
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, const SIndexPart& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	if (inStruct.sig != 0)
		assert((inStruct.sig == kIndexPartSig and inStruct.large_offsets == false) or (inStruct.sig == kIndexPartSigV2 and inStruct.large_offsets == true));
	
	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	data << kSIndexPartSize;
	data.Write(inStruct.name, sizeof(inStruct.name));
	data.Write(&inStruct.kind, sizeof(inStruct.kind));

	data << inStruct.bits_offset << inStruct.tree_offset
		 << inStruct.root << inStruct.entries
		 << inStruct.weight_offset;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SIndexPart& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));

	// backward compatibility code from here ...
	
	const uint32
		kIndexPartSigV1old = FOUR_CHAR_INLINE('indP'),
		kIndexPartSigV2old = FOUR_CHAR_INLINE('InDP');

	if (inStruct.sig == kIndexPartSigV1old)
		inStruct.sig = kIndexPartSig;
	else if (inStruct.sig == kIndexPartSigV2old)
		inStruct.sig = kIndexPartSigV2;
		
	// ... to here.
	
	if (inStruct.sig == kIndexPartSig)
		inStruct.large_offsets = false;
	else if (inStruct.sig == kIndexPartSigV2)
		inStruct.large_offsets = true;
	else
		THROW(("Incompatible index %4.4s", &inStruct.sig));
	
	data >> inStruct.size;
	data.Read(inStruct.name, sizeof(inStruct.name));
	data.Read(&inStruct.kind, sizeof(inStruct.kind));
	data >> inStruct.bits_offset >> inStruct.tree_offset
		 >> inStruct.root >> inStruct.entries
		 >> inStruct.weight_offset;
	
	if (inStruct.size != kSIndexPartSize)
	{
		SIndexPart t = {};
		memcpy(&t, &inStruct, min(inStruct.size, kSIndexPartSize));
		inStruct = t;
		
		int64 delta = kSIndexPartSize;
		delta -= inStruct.size;
		inData.Seek(-delta, SEEK_CUR);
	}
	
	return inData;
}

class CMergeWeightedDocInfo
{
  public:
					CMergeWeightedDocInfo(uint32 inDocDelta,
							CDbDocWeightIterator* inIterator)
						: fDelta(inDocDelta)
						, fIter(inIterator)
					{
						if (not Next())
							THROW(("Invalid weighted index, should contain at least one entry"));
					}
	
	bool			operator<(const CMergeWeightedDocInfo& inOther) const
						{ return Weight() < inOther.Weight() or (Weight() == inOther.Weight() and DocNr() > inOther.DocNr()); }
	
	bool			Next()
					{
						bool result = false;
						
						if (fIter != nil)
						{
							result = fIter->Next(fDocNr, fWeight, false);
							if (not result)
							{
								delete fIter;
								fIter = nil;
							}
						}
						
						return result;
					}
	
	uint32			DocNr() const			{ return fDocNr + fDelta; }
	uint8			Weight() const			{ return fWeight; }
	uint32			Count() const			{ return fIter->Count(); }
	
  private:
	uint32			fDelta;
	CDbDocWeightIterator*	fIter;
	uint32			fDocNr;
	uint8			fWeight;
};

// index

const uint32
	kRunBlockSize = 0x100000,		// 1 Mb
	kBufferEntryCount = 2000000;	// that's about 2.5 megabytes compressed

// CIndexBase is the base class for the various index types, these
// classes are only used for building indices.

class CIndexBase
{
  public:
					CIndexBase(const string& inName, uint32 inKind);
	virtual			~CIndexBase();
	
	virtual void	AddWord(const string& inWord, uint32 inFrequency = 1) = 0;
	virtual void	FlushDoc(uint32 inDocNr) = 0;
	virtual void	Write(HStreamBase& inDataFile, uint32 inDocCount, SIndexPart& outInfo) = 0;

	virtual bool	Empty() const = 0;
	
	virtual int		Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const;

  protected:
	string			fName;
  	uint32			fKind;
};

CIndexBase::CIndexBase(const string& inName, uint32 inKind)
	: fName(inName)
	, fKind(inKind)
{
}

CIndexBase::~CIndexBase()
{
}

int CIndexBase::Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const
{
	CIndexTraits<kTextIndex> comp;
	return comp.Compare(inA, inLengthA, inB, inLengthB);
}

// CValueIndex, can only store one value per document and so
// it should be unique

class CValueIndex : public CIndexBase
{
  public:
					CValueIndex(const string& inName);
	
	virtual void	AddWord(const string& inWord, uint32 inFrequency);
	virtual void	FlushDoc(uint32 inDocNr);
	virtual void	Write(HStreamBase& inDataFile, uint32 inDocCount, SIndexPart& outInfo);
	
	virtual bool	Empty() const			{ return false; }

  private:
	typedef map<string,uint32,CValueLess> ValueIndex;

	string			fWord;
	ValueIndex		fIndex;
};

CValueIndex::CValueIndex(const string& inName)
	: CIndexBase(inName, kValueIndex)
{
}
	
void CValueIndex::AddWord(const string& inWord, uint32 inFrequency)
{
	fWord = inWord;
}

void CValueIndex::FlushDoc(uint32 inDocNr)
{
	assert(fWord.length() < 256);

	if (fWord.length() == 0 and fName == "id")
	{
		cerr << "Warning, document nr " << inDocNr + 1 << " does not have an ID value" << endl;
		
		stringstream s;
		s << inDocNr;
		fWord = s.str();
	}

	if (fWord.length() > 0)
	{
		if (fIndex.count(fWord) > 0)
		{
			string w = fWord;
			
			stringstream s;
			s << w << "_" << inDocNr;
			fWord = s.str();
			
			cerr << "Attempt to enter duplicate value '" << w << "' in value index " << fName << endl;
			cerr << "The key will be replaced with '" << fWord << '\'' << endl;
		}
		
		(void)fIndex.insert(make_pair<string,uint32>(fWord, inDocNr));

		fWord.clear();
	}
}

void CValueIndex::Write(HStreamBase& inDataFile, uint32 /*inDocCount*/, SIndexPart& outInfo)
{
	if (VERBOSE >= 1)
	{
		cout << endl << "Writing index '" << fName << "'... ";
		cout.flush();
	}

	outInfo.sig = kIndexPartSig;
	outInfo.large_offsets = false;
	fName.copy(outInfo.name, 15);
	outInfo.kind = fKind;
	outInfo.tree_offset = inDataFile.Size();
	
	CIteratorWrapper<map<string,uint32,CValueLess> > iter(fIndex);
	auto_ptr<CIndex> n(CIndex::CreateFromIterator(kValueIndex, false, iter, inDataFile));

	outInfo.root = n->GetRoot();
	outInfo.entries = n->GetCount();

	if (VERBOSE >= 1)
		cout << "done" << endl << "Wrote " << outInfo.entries << " entries." << endl;
}

struct LexEntry
{
	const char*	word;
	uint32		nr;

				LexEntry()
					: word(nil), nr(0) {}
				LexEntry(const char* inWord, uint32 inNr)
					: word(inWord), nr(inNr) {}
				LexEntry(const LexEntry& inOther)
					: word(inOther.word), nr(inOther.nr) {}
	
	bool		operator<(const LexEntry& inOther) const
					{ return strcmp(word, inOther.word) < 0; }
	bool		operator==(const LexEntry& inOther) const
					{ return strcmp(word, inOther.word) == 0; }
};

class CFullTextIndex
{
	struct MergeInfo;

  public:
					CFullTextIndex(const HUrl& inScratch);
	virtual			~CFullTextIndex();
	
	void			ReleaseBuffer(uint32 inWeightBitCount);
	
	void			SetStopWords(const vector<string>& inStopWords);

	// two versions for AddWord, one works with lexicon's ID's
	void			AddWord(uint8 inIndex, const string& inWord, uint8 inFrequency);
	void			AddWord(uint8 inIndex, uint32 inWord, uint8 inFrequency);
	uint32			Store(const string& inWord)
					{
						return lexicon.Store(inWord);
					}

	void			FlushDoc(uint32 inDocNr, uint32 inWeightBitCount);
	
	void			Merge();
	
	// iterator functions
	// to iterate over the run info
	class CRunEntryIterator
	{
	  public:
						CRunEntryIterator(CFullTextIndex& inIndex, uint32 inWeightBitCount);
						~CRunEntryIterator();
		
		bool			Next(uint32& outDoc, uint32& outTerm, uint32& outIx, uint8& outWeight);
		int64			Count() const		{ return fEntryCount; }
	
	  private:
		CFullTextIndex&	fIndex;
		MergeInfo**		fMergeInfo;
		uint32			fRunCount;
		int64			fEntryCount;
	};

	friend class CRunEntryIterator;

	string			Lookup(uint32 inTerm);
	int				Compare(uint32 inTermA, uint32 inTermB) const
					{
						return lexicon.Compare(inTermA, inTermB);
					}

	bool			IsStopWord(uint32 inTerm) const
					{
						return fStopWords.count(inTerm) > 0;
					}

  private:
	
	void			FlushRun(uint32 inWeightBitCount);

	struct DocWord
	{
		uint32		word;
		uint32		index;
		uint32		freq;
		
		bool		operator<(const DocWord& inOther) const
					{
						return
							word < inOther.word or
							(word == inOther.word and index < inOther.index);
					}
	};

	typedef set<DocWord> DocWords;
	
	struct BufferEntry
	{
		uint32		term;
		uint32		doc;
		uint8		ix;
		uint8		weight;	// for weighted keys
		
		bool		operator<(const BufferEntry& inOther) const
						{ return term < inOther.term or (term == inOther.term and doc < inOther.doc); }
	};
	
	struct RunInfo
	{
		int64		offset;
		uint32		count;
	};
	typedef vector<RunInfo>	RunInfoArray;
	
	struct MergeInfo
	{
					MergeInfo(HStreamBase& inFile, int64 inOffset, uint32 inCount, uint32 inWeightBitCount)
						: cnt(inCount)
						, bits(inFile, inOffset)
						, term(0)
						, doc(0)
						, weight_bit_count(inWeightBitCount)
					{
						first_doc = ReadGamma(bits) - 1;
						doc = first_doc;
						Next();
					}

		void		Next()
					{
						uint32 td = ReadUnary(bits) - 1;
						if (td)
						{
							term += td;
							doc = first_doc;
						}

						doc += ReadGamma(bits) - 1;
						ix = ReadGamma(bits) - 1;
						weight = ReadBinary<uint8>(bits, weight_bit_count);

						--cnt;
					}

		uint32		cnt;
		CIBitStream	bits;
		uint32		term;
		uint32		doc;
		uint32		first_doc;
		uint32		ix;
		uint8		weight;
		uint32		weight_bit_count;
	};
	
	struct CompareMergeInfo
	{
		bool operator()(MergeInfo const* inA, MergeInfo const * inB) const
				{ return inA->term > inB->term or (inA->term == inB->term and inA->doc > inB->doc); }
	};

	typedef tr1::unordered_set<uint32, boost::hash<uint32> > CHashedSet;
	CHashedSet		fStopWords;

	CLexicon		lexicon;
	DocWords		fDocWords;
	uint32			fFTIndexCnt;
	BufferEntry*	buffer;
	uint32			buffer_ix;
	RunInfoArray	fRunInfo;
	HUrl			fScratchUrl;
	HStreamBase*	fScratch;
};

CFullTextIndex::CFullTextIndex(const HUrl& inScratchUrl)
	: fFTIndexCnt(0)
	, buffer(new BufferEntry[kBufferEntryCount])
	, buffer_ix(0)
	, fScratchUrl(inScratchUrl)
	, fScratch(new HTempFileStream(fScratchUrl))
{
}

CFullTextIndex::~CFullTextIndex()
{
	delete[] buffer;
	delete fScratch;
}

void CFullTextIndex::SetStopWords(const vector<string>& inStopWords)
{
	fStopWords.clear();
	for (vector<string>::const_iterator sw = inStopWords.begin(); sw != inStopWords.end(); ++sw)
		fStopWords.insert(Store(*sw));
}

void CFullTextIndex::AddWord(uint8 inIndex, const string& inWord, uint8 inFrequency)
{
	assert(inWord.length() > 0);
	
	AddWord(inIndex, lexicon.Store(inWord), inFrequency);
}

void CFullTextIndex::AddWord(uint8 inIndex, uint32 inWord, uint8 inFrequency)
{
	if (inIndex > kMaxIndexNr)
		THROW(("Too many full text indices"));
	
	if (fFTIndexCnt < inIndex + 1)
		fFTIndexCnt = inIndex + 1;

	if (inFrequency > 0 and not IsStopWord(inWord))
	{
		DocWord w = { inWord, inIndex, inFrequency };
		
		DocWords::iterator i = fDocWords.find(w);
		if (i != fDocWords.end())
			const_cast<DocWord&>(*i).freq += inFrequency;
		else
			fDocWords.insert(w);
	}
}

void CFullTextIndex::FlushDoc(uint32 inDoc, uint32 inWeightBitCount)
{
	if (buffer_ix + fDocWords.size() >= kBufferEntryCount)
		FlushRun(inWeightBitCount);

	// normalize the frequencies.
	
	float maxFreq = 1;
	uint32 kMaxWeight = (1 << inWeightBitCount) - 1;
	
	for (DocWords::iterator w = fDocWords.begin(); w != fDocWords.end(); ++w)
		if (w->freq > maxFreq)
			maxFreq = w->freq;

	for (DocWords::iterator w = fDocWords.begin(); w != fDocWords.end(); ++w)
	{
		if (w->freq == 0)
			continue;
		
		buffer[buffer_ix].term = w->word;
		buffer[buffer_ix].doc = inDoc;
		buffer[buffer_ix].ix = w->index;

		float wf = static_cast<float>(w->freq) / maxFreq;
		buffer[buffer_ix].weight = static_cast<uint32>(wf * kMaxWeight);
		if (buffer[buffer_ix].weight < 1)
			buffer[buffer_ix].weight = 1;
	
		assert(buffer[buffer_ix].weight <= kMaxWeight);
		assert(buffer[buffer_ix].weight > 0);		

		++buffer_ix;
	}
	
	fDocWords.clear();
}

void CFullTextIndex::FlushRun(uint32 inWeightBitCount)
{
	uint32 firstDoc = buffer[0].doc;	// the first doc in this run
	
	sort(buffer, buffer + buffer_ix);

	RunInfo ri;
	ri.offset = fScratch->Seek(0, SEEK_END);
	ri.count = buffer_ix;
	fRunInfo.push_back(ri);
	
	COBitStream bits(*fScratch);

	uint32 t = 0;
	uint32 d = firstDoc;	// the first doc in this run
	
	WriteGamma(bits, d + 1);

	for (uint32 i = 0; i < buffer_ix; ++i)
	{
		if (buffer[i].term > t)
			d = firstDoc;

		WriteUnary(bits, (buffer[i].term - t) + 1);
		WriteGamma(bits, (buffer[i].doc - d) + 1);
		WriteGamma(bits, buffer[i].ix + 1);

		assert(buffer[i].weight > 0);
		assert(buffer[i].weight <= ((1 << inWeightBitCount) - 1));

		WriteBinary(bits, buffer[i].weight, inWeightBitCount);
		
		t = buffer[i].term;
		d = buffer[i].doc;
	}
	
	bits.sync();

	buffer_ix = 0;
}

void CFullTextIndex::ReleaseBuffer(uint32 inWeightBitCount)
{
	FlushRun(inWeightBitCount);
	delete[] buffer;
	buffer = NULL;
	
	if (VERBOSE >= 1)
	{
		cout << endl << "Wrote " << fRunInfo.size() << " info runs" << endl;
		cout << endl << "Lexicon contains " << lexicon.Count() << " entries" << endl;
	}
}

CFullTextIndex::CRunEntryIterator::CRunEntryIterator(
		CFullTextIndex& inIndex, uint32 inWeightBitCount)
	: fIndex(inIndex)
{
	fRunCount = fIndex.fRunInfo.size();
	fMergeInfo = new MergeInfo*[fRunCount];

	fEntryCount = 0;
	for (RunInfoArray::iterator ri = fIndex.fRunInfo.begin(); ri != fIndex.fRunInfo.end(); ++ri)
	{
		MergeInfo* mi = new MergeInfo(*fIndex.fScratch, (*ri).offset, (*ri).count, inWeightBitCount);
		fMergeInfo[ri - fIndex.fRunInfo.begin()] = mi;
		fEntryCount += mi->cnt + 1;
	}
	
	make_heap(fMergeInfo, fMergeInfo + fRunCount, CompareMergeInfo());
}

CFullTextIndex::CRunEntryIterator::~CRunEntryIterator()
{
	assert(fRunCount == 0);
	
	while (fRunCount-- > 0)
		delete fMergeInfo[fRunCount];
	
	delete[] fMergeInfo;
}

bool CFullTextIndex::CRunEntryIterator::Next(uint32& outDoc, uint32& outTerm, uint32& outIx, uint8& outWeight)
{
	bool result = false;
	
	if (fRunCount > 0)
	{
		if (fRunCount > 1)
			pop_heap(fMergeInfo, fMergeInfo + fRunCount, CompareMergeInfo());
		
		MergeInfo* cur = fMergeInfo[fRunCount - 1];
		
		outDoc = cur->doc;
		outTerm = cur->term;
		outIx = cur->ix;
		outWeight = cur->weight;
		
		if (cur->cnt != 0)
		{
			cur->Next();
			push_heap(fMergeInfo, fMergeInfo + fRunCount, CompareMergeInfo());
		}
		else
		{
			delete cur;
			--fRunCount;
			fMergeInfo[fRunCount] = nil;
		}

		result = true;
		--fEntryCount;
	}
	
	return result;
}

string CFullTextIndex::Lookup(uint32 inTerm)
{
	return lexicon.GetString(inTerm);
}

class CTextIndexBase : public CIndexBase
{
  public:
					CTextIndexBase(CFullTextIndex& inFullTextIndex,
						const string& inName, uint16 inIndexNr,
						const HUrl& inScratch, CIndexKind inKind,
						uint32 inWeightBitCount);
					~CTextIndexBase();
	
	virtual void	AddWord(const string& inWord, uint32 inFrequency);
	virtual void	FlushDoc(uint32 inDocNr);
	
	virtual void	AddDocTerm(uint32 inDoc, uint8 inFrequency);
	virtual void	FlushTerm(uint32 inTerm, uint32 inDocCount);
	
	virtual void	Write(HStreamBase& inDataFile, uint32 inDocCount, SIndexPart& outInfo);

					// NOTE: empty is overloaded (i.e. used for two purposes)
	virtual bool	Empty() const			{ return fEmpty; }
	uint16			GetIxNr() const			{ return fIndexNr; }

  protected:

	CFullTextIndex&	fFullTextIndex;
	uint16			fIndexNr;
	bool			fEmpty;
	uint32			fWeightBitCount;
	
	// data for the second pass
	uint32			fLastDoc;
	COBitStream*	fBits;
	uint32			fDocCount;
	
	HUrl			fBitUrl;
	HStreamBase*	fBitFile;
	HMemoryStream	fScratch;
	
	COBitStream*	fRawIndex;
	uint32			fRawIndexCnt;
	uint32			fLastTermNr;
	uint64			fLastOffset;
};

CTextIndexBase::CTextIndexBase(CFullTextIndex& inFullTextIndex,
		const string& inName, uint16 inIndexNr, const HUrl& inScratch,
		CIndexKind inKind, uint32 inWeightBitCount)
	: CIndexBase(inName, inKind)
	, fFullTextIndex(inFullTextIndex)
	, fIndexNr(inIndexNr)
	, fEmpty(true)
	, fWeightBitCount(inWeightBitCount)
	, fLastDoc(0)
	, fBits(nil)
	, fDocCount(0)
	, fBitUrl(inScratch)
	, fRawIndex(nil)
	, fRawIndexCnt(0)
	, fLastTermNr(0)
	, fLastOffset(0)
{
	fBitFile = new HTempFileStream(fBitUrl);
}

CTextIndexBase::~CTextIndexBase()
{
	delete fBits;
	delete fRawIndex;
	delete fBitFile;
}

void CTextIndexBase::AddWord(const string& inWord, uint32 inFrequency)
{
	fFullTextIndex.AddWord(fIndexNr, inWord, inFrequency);
	fEmpty = false;
}

void CTextIndexBase::FlushDoc(uint32 /*inDocNr*/)
{
}

void CTextIndexBase::AddDocTerm(uint32 inDoc, uint8 inFrequency)
{
	uint32 d;

	if (fBits == nil)
	{
		fBits = new COBitStream(fScratch);
		fDocCount = 0;
		d = inDoc + 1;
	}
	else
		d = inDoc - fLastDoc;
	
	assert(d > 0);

	WriteGamma(*fBits, d);
	
	if (fWeightBitCount > 0)
	{
		uint32 kMaxWeight = ((1 << fWeightBitCount) - 1);
		
		if (inFrequency < 1)
			inFrequency = 1;
		else if (inFrequency >= kMaxWeight)
			inFrequency = kMaxWeight;
		
		WriteBinary(*fBits, inFrequency, fWeightBitCount);
	}
	
	fLastDoc = inDoc;
	++fDocCount;
	fEmpty = false;
}

void CTextIndexBase::FlushTerm(uint32 inTerm, uint32 inDocCount)
{
//#if P_DEBUG
//	cout << fFullTextIndex.Lookup(inTerm) << endl;
//#endif
	
	if (fDocCount > 0 and fBits != nil)
	{
		// flush the raw index bits
		fBits->sync();
		
		int64 offset = fBitFile->Seek(0, SEEK_END);
		if (offset > numeric_limits<uint32>::max())
			THROW(("Resulting full text index is too large"));
		
		if (fRawIndex == nil)
		{
			fRawIndex = new COBitStream;
			fRawIndexCnt = 1;
		
			WriteGamma(*fRawIndex, inTerm + 1);
			fLastTermNr = inTerm;
		
			WriteGamma(*fRawIndex, offset + 1);
			fLastOffset = offset;
		}
		else
		{
			uint32 delta = inTerm - fLastTermNr;
			fLastTermNr = inTerm;
			WriteGamma(*fRawIndex, delta);
		
			delta = offset - fLastOffset;
			fLastOffset = offset;
			WriteGamma(*fRawIndex, delta);

			++fRawIndexCnt;
		}
		
		COBitStream docBits(*fBitFile);

		if (fWeightBitCount > 0)
		{
			vector<pair<uint32,uint8> > docs;
			
			CIBitStream bits(fScratch, 0, fScratch.Size());

			// first written value is offset by 1, correct here
			uint32 docNr = ReadGamma(bits) - 1;
			uint8 weight = ReadBinary<uint8>(bits, fWeightBitCount);
			
			assert(weight > 0);
//			assert(weight <= kMaxWeight);

			docs.push_back(make_pair(docNr, weight));

			for (uint32 d = 1; d < fDocCount; ++d)
			{
				docNr += ReadGamma(bits);
				weight = ReadBinary<uint8>(bits, fWeightBitCount);

				assert(weight > 0);
//				assert(weight <= kMaxWeight);

				docs.push_back(make_pair(docNr, weight));
			}

			CompressArray(docBits, docs, inDocCount);
		}
		else
		{
			vector<uint32> docs;
			
			CIBitStream bits(fScratch, 0, fScratch.Size());

			// first written value is offset by 1, correct here
			uint32 docNr = ReadGamma(bits) - 1;
			docs.push_back(docNr);
			
			for (uint32 d = 1; d < fDocCount; ++d)
			{
				docNr += ReadGamma(bits);
				docs.push_back(docNr);
			}

			CompressArray(docBits, docs, inDocCount);
		}

		docBits.sync();
	}

	delete fBits;
	fBits = nil;
	fDocCount = 0;
	fLastDoc = 0;
	fScratch.Truncate(0);
	fEmpty = true;
}

struct SortLex
{
			SortLex(CFullTextIndex& inFT, CIndexBase* inIndexBase)
				: fFT(inFT), fBase(inIndexBase) {}
	
	inline
	bool	operator()(const pair<uint32,uint32>& inA, const pair<uint32,uint32>& inB) const
			{
				string a = fFT.Lookup(inA.first);
				string b = fFT.Lookup(inB.first);
				return fBase->Compare(a.c_str(), a.length(), b.c_str(), b.length()) < 0;
//				return fFT.Compare(inA.first, inB.first) < 0;
			}
	
	CFullTextIndex&	fFT;
	CIndexBase*		fBase;
};

class CFullTextIterator : public CIteratorBase
{
  public:
						CFullTextIterator(CFullTextIndex& inFullTextIndex,
							vector<pair<uint32,int64> >& inLexicon);

	virtual bool		Next(string& outString, int64& outValue);

  private:
	CFullTextIndex&							fFullText;
	vector<pair<uint32,int64> >::iterator	fCurrent, fEnd;
};

CFullTextIterator::CFullTextIterator(CFullTextIndex& inFullTextIndex,
		vector<pair<uint32,int64> >& inLexicon)
	: fFullText(inFullTextIndex)
	, fCurrent(inLexicon.begin())
	, fEnd(inLexicon.end())
{
}

bool CFullTextIterator::Next(string& outKey, int64& outValue)
{
	bool result = false;
	if (fCurrent != fEnd)
	{
		result = true;

		outKey = fFullText.Lookup(fCurrent->first);
		outValue = fCurrent->second;
		
		++fCurrent;
	}
	return result;
}

void CTextIndexBase::Write(HStreamBase& inDataFile, uint32 /*inDocCount*/, SIndexPart& outInfo)
{
	if (VERBOSE >= 1)
	{
		cout << endl << "Writing index '" << fName << "' nr: " << fIndexNr << "... ";
		cout.flush();
	}

	outInfo.sig = kIndexPartSig;
	fName.copy(outInfo.name, 15);
	outInfo.kind = fKind;
	outInfo.bits_offset = inDataFile.Seek(0, SEEK_END);

	outInfo.large_offsets = false;
#if P_DEBUG
	outInfo.sig = kIndexPartSigV2;
	outInfo.large_offsets = true;
#endif
	
	if (fBitFile->Size() >= numeric_limits<uint32>::max())
	{
		outInfo.sig = kIndexPartSigV2;
		outInfo.large_offsets = true;
	}
	
	// copy the bits to the data file
	const int kBufSize = 4096;
	HAutoBuf<char> b(new char[kBufSize]);
	int64 k = fBitFile->Size();
	fBitFile->Seek(0, SEEK_SET);
	while (k > 0)
	{
		uint32 n = kBufSize;
		if (n > k)
			n = k;
		fBitFile->Read(b.get(), n);
		inDataFile.Write(b.get(), n);
		k -= n;
	}

	// construct the optimized b-tree
	outInfo.tree_offset = inDataFile.Seek(0, SEEK_END);

	vector<pair<uint32,int64> > lexicon;
	lexicon.reserve(fRawIndexCnt);
	
	fRawIndex->sync();
	CIBitStream bits(fRawIndex->peek(), fRawIndex->size());
	
	uint32 term;
	int64 offset;
	term = ReadGamma(bits) - 1;
	offset = ReadGamma(bits) - 1;
	
	lexicon.push_back(make_pair(term, offset));
	--fRawIndexCnt;
	while (fRawIndexCnt-- > 0)
	{
		term += ReadGamma(bits);
		offset += ReadGamma(bits);
		lexicon.push_back(make_pair(term, offset));
	}

	sort(lexicon.begin(), lexicon.end(), SortLex(fFullTextIndex, this));

	CFullTextIterator iter(fFullTextIndex, lexicon);
	auto_ptr<CIndex> indx(CIndex::CreateFromIterator(fKind, outInfo.large_offsets, iter, inDataFile));

	outInfo.root = indx->GetRoot();
	outInfo.entries = lexicon.size();
	
	if (VERBOSE >= 1)
		cout << "done" << endl << "Wrote " << lexicon.size() << " entries." << endl;
}

class CTextIndex : public CTextIndexBase
{
  public:
					CTextIndex(CFullTextIndex& inFullTextIndex,
						const string& inName, uint16 inIndexNr,
						const HUrl& inScratch);
};

CTextIndex::CTextIndex(CFullTextIndex& inFullTextIndex,
		const string& inName, uint16 inIndexNr, const HUrl& inScratch)
	: CTextIndexBase(inFullTextIndex, inName, inIndexNr, inScratch, kTextIndex, 0)
{
}

class CWeightedWordIndex : public CTextIndexBase
{
  public:
					CWeightedWordIndex(CFullTextIndex& inFullTextIndex,
						const string& inName, uint16 inIndexNr,
						const HUrl& inScratch, uint32 inWeightBitCount);

	virtual void	Write(HStreamBase& inDataFile, uint32 inDocCount, SIndexPart& outInfo);
};

CWeightedWordIndex::CWeightedWordIndex(CFullTextIndex& inFullTextIndex,
		const string& inName, uint16 inIndexNr, const HUrl& inScratch, uint32 inWeightBitCount)
	: CTextIndexBase(inFullTextIndex, inName, inIndexNr, inScratch, kWeightedIndex, inWeightBitCount)
{
}

void CWeightedWordIndex::Write(HStreamBase& inDataFile, uint32 inDocCount, SIndexPart& outInfo)
{
	CTextIndexBase::Write(inDataFile, inDocCount, outInfo);
	
	outInfo.weight_offset = inDataFile.Seek(0, SEEK_END);
	
	// allocate space for the weight vector
	const int kBufSize = 4096;
	HAutoBuf<char> b(new char[kBufSize]);
	memset(b.get(), 0, kBufSize);
	
	int64 k = inDocCount * sizeof(float);

	while (k > 0)
	{
		uint32 n = kBufSize;
		if (n > k)
			n = k;
		inDataFile.Write(b.get(), n);
		k -= n;
	}
}

class CAllTextIndex : public CWeightedWordIndex
{
  public:
					CAllTextIndex(CFullTextIndex& inFullTextIndex,
						uint16 inIndexNr, const HUrl& inScratch,
						uint32 inWeightBitCount);						
	
//	virtual void	AddDocTerm(uint32 inDoc, uint8 inFrequency);
//	virtual void	FlushTerm(uint32 inTerm, uint32 inDocCount);

  private:
	uint32			fDocNr;
	uint32			fFreq;
};

CAllTextIndex::CAllTextIndex(CFullTextIndex& inFullTextIndex,
		uint16 inIndexNr, const HUrl& inScratch, uint32 inWeightBitCount)
		: CWeightedWordIndex(inFullTextIndex, kAllTextIndexName, inIndexNr, inScratch, inWeightBitCount)
	, fDocNr(0)
	, fFreq(0)
{
}

//void CAllTextIndex::AddDocTerm(uint32 inDoc, uint8 inFrequency)
//{
//	if (inDoc == fDocNr)
//		fFreq += inFrequency;
//	else
//	{
////		CTextIndexBase::AddDocTerm(fDocNr, fFreq);
//		fDocNr = inDoc;
//		fFreq = inFrequency;
//	}
//}

//void CAllTextIndex::FlushTerm(uint32 inTerm, uint32 inDocCount)
//{
//	if (fFreq != 0)
//		CTextIndexBase::AddDocTerm(fDocNr, fFreq);
//
//	fDocNr = 0;
//	fFreq = 0;
//	
//	CTextIndexBase::FlushTerm(inTerm, inDocCount);
//}

class CDateIndex : public CTextIndexBase
{
  public:
					CDateIndex(CFullTextIndex& inFullTextIndex,
						const string& inName, uint16 inIndexNr,
						const HUrl& inScratch);
};

CDateIndex::CDateIndex(CFullTextIndex& inFullTextIndex,
		const string& inName, uint16 inIndexNr, const HUrl& inScratch)
	: CTextIndexBase(inFullTextIndex, inName, inIndexNr, inScratch, kDateIndex, 0)
{
}

class CNumberIndex : public CTextIndexBase
{
  public:
					CNumberIndex(CFullTextIndex& inFullTextIndex,
						const string& inName, uint16 inIndexNr,
						const HUrl& inScratch);

	virtual int		Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const;
};

CNumberIndex::CNumberIndex(CFullTextIndex& inFullTextIndex,
		const string& inName, uint16 inIndexNr, const HUrl& inScratch)
	: CTextIndexBase(inFullTextIndex, inName, inIndexNr, inScratch, kNumberIndex, 0)
{
}

int CNumberIndex::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	CIndexTraits<kNumberIndex> comp;
	return comp.Compare(inA, inLengthA, inB, inLengthB);
}

// 

CIndexer::CIndexer(const HUrl& inDb)
	: fDb(inDb.GetURL())
	, fFullTextIndex(nil)
	, fNextTextIndexID(0)
	, fFile(nil)
	, fHeader(new SIndexHeader)
	, fParts(nil)
	, fOffset(0)
	, fSize(0)
{
	fHeader->sig = kIndexSig;
	fHeader->count = 0;
	fHeader->entries = 0;
	fHeader->weight_bit_count = WEIGHT_BIT_COUNT;
}

CIndexer::CIndexer(HStreamBase& inFile, int64 inOffset, int64 inSize)
	: fFullTextIndex(nil)
	, fNextTextIndexID(0)
	, fFile(&inFile)
	, fOffset(inOffset)
	, fSize(inSize)
{
	HStreamView view(inFile, inOffset, inSize);

	fHeader = new SIndexHeader;
	view >> *fHeader;
	
	fParts = new SIndexPart[fHeader->count];
	for (uint32 i = 0; i < fHeader->count; ++i)
	{
		view >> fParts[i];
		
		if (strcmp(fParts[i].name, "*alltext*") == 0)
			strcpy(fParts[i].name, kAllTextIndexName);
	}
}

CIndexer::~CIndexer()
{
	delete fHeader;
	delete[] fParts;
	
	delete fFullTextIndex;
}

void CIndexer::SetStopWords(const vector<string>& inStopWords)
{
	GetFullTextIndex().SetStopWords(inStopWords);
}

void CIndexer::IndexText(const string& inIndex, const string& inText, bool inIndexNrs)
{
	if (inText.length() == 0)
		return;
	
	CIndexBase* index = indexes[inIndex];
	if (index == NULL)
	{
		HUrl url(fDb + '.' + inIndex + "_indx");
		index = indexes[inIndex] =
			new CTextIndex(GetFullTextIndex(), inIndex, fNextTextIndexID++, url);
	}
	else if (dynamic_cast<CTextIndex*>(index) == NULL)
		THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));

	CTokenizer<255> tok(inText.c_str(), inText.length());
	bool isWord, isNumber;
	CTokenizer<255>::EntryText s;
	
	while (tok.GetToken(s, isWord, isNumber))
	{
		if (not (isWord or isNumber) or s[0] == 0)
			continue;
		
		if (isNumber and not inIndexNrs)
			continue;

		char* c;
		for (c = s; *c; ++c)
			*c = static_cast<char>(tolower(*c));

		if (c - s < kMaxKeySize)
			index->AddWord(s);
	}
}

void CIndexer::IndexText(const string& inIndex, const string& inText)
{
	IndexText(inIndex, inText, false);
}

void CIndexer::IndexTextAndNumbers(const string& inIndex, const string& inText)
{
	IndexText(inIndex, inText, true);
}

void CIndexer::IndexWord(const string& inIndex, const string& inText)
{
	if (inText.length() > 0)
	{
		CIndexBase* index = indexes[inIndex];
		if (index == NULL)
		{
			HUrl url(fDb + '.' + inIndex + "_indx");
			index = indexes[inIndex] =
				new CTextIndex(GetFullTextIndex(), inIndex, fNextTextIndexID++, url);
		}
		else if (dynamic_cast<CTextIndex*>(index) == NULL)
			THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));
	
		index->AddWord(inText);
	}
}

void CIndexer::IndexDate(const string& inIndex, const string& inText)
{
	// inText is a formatted date which must conform to:
	// YYYY-MM-DD
	// Make sure it is a valid date first...
	
	uint32 year, month, day;
	char *p;

	if (inText.length() != 10)
		THROW(("Invalid formatted date specified(1): '%s'", inText.c_str()));
	
	year = strtoul(inText.c_str(), &p, 10);
	if (p == nil or *p != '-')
		THROW(("Invalid formatted date specified(2): '%s'", inText.c_str()));
	month = strtoul(p + 1, &p, 10);
	if (p == nil or *p != '-' or month < 1 or month > 12)
		THROW(("Invalid formatted date specified(3): '%s'", inText.c_str()));
	day = strtoul(p + 1, &p, 10);
	if (not (p == nil or *p == 0) or day < 1 or day > 31)
		THROW(("Invalid formatted date specified(4): '%s'", inText.c_str()));
	
	struct tm tm = { 0 };
	tm.tm_mday = day;
	tm.tm_mon = month - 1;
	tm.tm_year = year - 1900;
#if P_VISUAL_CPP
#pragma message("TODO: fix me!")
	time_t t = mktime(&tm);

//	if (localtime_r(&t, &tm) == nil)
//		THROW(("Invalid formatted date specified(5): '%s'", inText.c_str()));
#else
	time_t t = timegm(&tm);

	if (gmtime_r(&t, &tm) == nil)
		THROW(("Invalid formatted date specified(5): '%s'", inText.c_str()));
#endif
	
	if (VERBOSE > 0)
	{
		if (tm.tm_mday != day or tm.tm_mon != month - 1 or tm.tm_year != year - 1900)
			cerr << "Warning: Invalid formatted date specified(6): " << inText << endl;
		//THROW(("Invalid formatted date specified(6): '%s'", inText.c_str()));
	}

	// OK, so the date specified seems to be valid, now store it as a text string

	if (inText.length() > 0)
	{
		CIndexBase* index = indexes[inIndex];
		if (index == NULL)
		{
			HUrl url(fDb + '.' + inIndex + "_indx");
			index = indexes[inIndex] =
				new CDateIndex(GetFullTextIndex(), inIndex, fNextTextIndexID++, url);
		}
		else if (dynamic_cast<CDateIndex*>(index) == NULL)
			THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));
	
		index->AddWord(inText);
	}
}

void CIndexer::IndexNumber(const string& inIndex, const string& inText)
{
	// inText should be an integer i.e. only consisting of digits.
	
	for (string::const_iterator ch = inText.begin(); ch != inText.end(); ++ch)
	{
		if (not isdigit(*ch))
			THROW(("Value passed to IndexNumber is not a valid integer: '%s'", inText.c_str()));
	}

	if (inText.length() > 0)
	{
		CIndexBase* index = indexes[inIndex];
		if (index == NULL)
		{
			HUrl url(fDb + '.' + inIndex + "_indx");
			index = indexes[inIndex] =
				new CNumberIndex(GetFullTextIndex(), inIndex, fNextTextIndexID++, url);
		}
		else if (dynamic_cast<CNumberIndex*>(index) == NULL)
			THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));
	
		index->AddWord(inText);
	}
}

void CIndexer::IndexValue(const string& inIndex, const string& inText)
{
	CIndexBase* index = indexes[inIndex];
	if (index == NULL)
		index = indexes[inIndex] = new CValueIndex(inIndex);
	else if (dynamic_cast<CValueIndex*>(index) == NULL)
		THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));

	if (inText.length() >= kMaxKeySize)
		THROW(("Data error: length of unique key too long (%s)", inText.c_str()));

	index->AddWord(inText);
}

void CIndexer::IndexWordWithWeight(const string& inIndex, const string& inText, uint32 inFrequency)
{
	CIndexBase* index = indexes[inIndex];
	if (index == NULL)
	{
		HUrl url(fDb + '.' + inIndex + "_indx");
		index = indexes[inIndex] = new CWeightedWordIndex(
			GetFullTextIndex(), inIndex, fNextTextIndexID++, url, fHeader->weight_bit_count);
	}
	else if (dynamic_cast<CWeightedWordIndex*>(index) == NULL)
		THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));

	if (inText.length() >= kMaxKeySize)
		THROW(("Data error: length of unique key too long (%s)", inText.c_str()));

	index->AddWord(inText, inFrequency);
}

void CIndexer::FlushDoc()
{
	if (fFullTextIndex)
		fFullTextIndex->FlushDoc(fHeader->entries, fHeader->weight_bit_count);

	map<string,CIndexBase*>::iterator indx;

	for (indx = indexes.begin(); indx != indexes.end(); ++indx)
		(*indx).second->FlushDoc(fHeader->entries);
		
	++fHeader->entries;
}

void CIndexer::CreateIndex(HStreamBase& inFile,
	int64& outOffset, int64& outSize, bool inCreateAllTextIndex)
{
	if (fParts)
		delete[] fParts;
	
	fParts = nil;
	fFile = &inFile;

	fHeader->sig = kIndexSig;
	fHeader->count = 0;
	
	map<string,CIndexBase*>::iterator indx;
	
	for (indx = indexes.begin(); indx != indexes.end(); ++indx)
	{
		if ((*indx).second->Empty())
		{
			delete (*indx).second;
			(*indx).second = nil;
		}
		else
			++fHeader->count;
	}

	if (inCreateAllTextIndex)
		fHeader->count += 1; // for our allIndex

	if (fFullTextIndex)
		fFullTextIndex->ReleaseBuffer(fHeader->weight_bit_count);
	
	outOffset = inFile.Seek(0, SEEK_END);
	inFile << *fHeader;
	
	fParts = new SIndexPart[fHeader->count];
	memset(fParts, 0, sizeof(SIndexPart) * fHeader->count);
	inFile.Write(fParts, sizeof(SIndexPart) * fHeader->count);

	// optimize invariant
	uint32 textIndexCount = fNextTextIndexID;

	HAutoBuf<CTextIndexBase*> txtIndexBuf(new CTextIndexBase*[textIndexCount]);
	CTextIndexBase** txtIndex = txtIndexBuf.get();
	memset(txtIndex, 0, sizeof(CTextIndex*) * textIndexCount);

	for (indx = indexes.begin(); indx != indexes.end(); ++indx)
	{
		if (dynamic_cast<CTextIndexBase*>((*indx).second) != nil)
		{
			CTextIndexBase* txtIx = static_cast<CTextIndexBase*>((*indx).second);
			txtIndex[txtIx->GetIxNr()] = txtIx;
		}
	}
	
	CTextIndexBase* allIndex = nil;
	if (inCreateAllTextIndex)
	{
		HUrl url(fDb + '.' + "alltext_indx");
		allIndex = new CAllTextIndex(GetFullTextIndex(), fHeader->count - 1, url, fHeader->weight_bit_count);
	}
	
	if (fFullTextIndex)
	{
		if (VERBOSE > 0)
		{
			cout << endl << "Creating full text indexes... ";
			cout.flush();
		}

		uint32 iDoc, lDoc, iTerm, iIx, lTerm = 0, i, tFreq;
		uint8 iFreq;
		CFullTextIndex::CRunEntryIterator iter(*fFullTextIndex, fHeader->weight_bit_count);
	
		// the next loop is very *hot*, make sure it is optimized as much as possible
		if (iter.Next(iDoc, iTerm, iIx, iFreq))
		{
			lTerm = iTerm;
			lDoc = iDoc;
			tFreq = iFreq;

			bool isStopWord = fFullTextIndex->IsStopWord(iTerm);

			do
			{
				if (allIndex and (lDoc != iDoc or lTerm != iTerm))
				{
					if (not isStopWord)
						allIndex->AddDocTerm(lDoc, tFreq);

					lDoc = iDoc;
					tFreq = 0;
				}
				
				if (lTerm != iTerm)
				{
					for (i = 0; i < textIndexCount; ++i)
					{
						if (txtIndex[i] != nil and not txtIndex[i]->Empty())
							txtIndex[i]->FlushTerm(lTerm, fHeader->entries);
					}
					
					if (allIndex and not isStopWord)
						allIndex->FlushTerm(lTerm, fHeader->entries);

					lTerm = iTerm;
					tFreq = 0;
					isStopWord = fFullTextIndex->IsStopWord(iTerm);
				}
				
				txtIndex[iIx]->AddDocTerm(iDoc, iFreq);

				tFreq += iFreq;
			}
			while (iter.Next(iDoc, iTerm, iIx, iFreq));
		}
	
		for (i = 0; i < textIndexCount; ++i)
		{
			if (txtIndex[i] != nil)
				txtIndex[i]->FlushTerm(lTerm, fHeader->entries);
		}

		if (allIndex and not allIndex->Empty())
			allIndex->FlushTerm(lTerm, fHeader->entries);

		if (VERBOSE > 0)
			cout << "done" << endl;
	}
	
	uint32 ix = 0;
	for (indx = indexes.begin(); indx != indexes.end(); ++indx)
	{
		if ((*indx).second != nil)
		{
			(*indx).second->Write(inFile, fHeader->entries, fParts[ix++]);
			delete (*indx).second;
		}
	}

	if (allIndex != nil)
	{
		allIndex->Write(inFile, fHeader->entries, fParts[ix++]);
		delete allIndex;
	}
	
	outSize = inFile.Seek(0, SEEK_END) - outOffset;
	
	inFile.Seek(outOffset + kSIndexHeaderSize, SEEK_SET);
	for (ix = 0; ix < fHeader->count; ++ix)
		inFile << fParts[ix];
}

struct CMergeData
{
						CMergeData()
							: count(0)
							, indx(nil)
							, data(nil)
						{
						}

	vector<SIndexPart>	info;
	uint32				count;
	
	// cached data:
	CIndex*				indx;
	HStreamBase*		data;
};

struct CCompareIndexInfo
{
	bool operator()(const SIndexPart& inA, const SIndexPart& inB) const
	{
		return strcmp(inA.name, inB.name) < 0;
	}
};

// the next class is needed because we run out of memory with huge indices.

class CMergeIndexBuffer
{
	typedef pair<string,int64>	value_type;
	
  public:
						CMergeIndexBuffer(const string& inBaseUrl);
						~CMergeIndexBuffer();

	void				push_back(const value_type& inData);

	struct iterator : public boost::iterator_facade<iterator,
		const value_type, boost::forward_traversal_tag>
	{
						iterator(const iterator& inOther);
		iterator&		operator=(const iterator& inOther);
		
	  private:
		friend class boost::iterator_core_access;
		friend class CMergeIndexBuffer;

						iterator(HStreamBase& inFile, int64 inOffset);
	
		void			increment();
		void			decrement() { assert(false); }
		bool			equal(const iterator& inOther) const;
		reference		dereference() const;

		void			read();
		
		HStreamBase*	fFile;
		int64			fOffset;
		value_type		fCurrent;
	};
	
	iterator			begin()			{ if (fBufferIndx) FlushBuffer(); return iterator(fFile, 0); }
	iterator			end()			{ if (fBufferIndx) FlushBuffer(); return iterator(fFile, fFile.Size()); }
	
  private:	

	void				FlushBuffer();

	static const uint32	kBufferSize;
	HTempFileStream		fFile;
	value_type*			fBuffer;
	uint32				fBufferIndx;
};

const uint32 CMergeIndexBuffer::kBufferSize = 1024;

CMergeIndexBuffer::CMergeIndexBuffer(const string& inBaseUrl)
	: fFile(HUrl(inBaseUrl + "_merge_indx"))
	, fBuffer(new value_type[kBufferSize])
	, fBufferIndx(0)
{	
}

CMergeIndexBuffer::~CMergeIndexBuffer()
{
	delete[] fBuffer;
}

void CMergeIndexBuffer::push_back(const value_type& inValue)
{
	if (fBufferIndx >= kBufferSize)
		FlushBuffer();
	
	fBuffer[fBufferIndx] = inValue;
	++fBufferIndx;
}

void CMergeIndexBuffer::FlushBuffer()
{
	for (uint32 ix = 0; ix < fBufferIndx; ++ix)
		fFile << fBuffer[ix].first << fBuffer[ix].second;
	fBufferIndx = 0;
}

CMergeIndexBuffer::iterator::iterator(HStreamBase& inFile, int64 inOffset)
	: fFile(&inFile)
	, fOffset(inOffset)
{
	read();
}

CMergeIndexBuffer::iterator::iterator(const iterator& inOther)
	: fFile(inOther.fFile)
	, fOffset(inOther.fOffset)
	, fCurrent(inOther.fCurrent)
{
}

CMergeIndexBuffer::iterator& CMergeIndexBuffer::iterator::operator=(const iterator& inOther)
{
	fFile = inOther.fFile;
	fOffset = inOther.fOffset;
	fCurrent = inOther.fCurrent;
	return *this;
}

void CMergeIndexBuffer::iterator::increment()
{
	if (fCurrent.first.length() == 0)
		read();

	fOffset += fCurrent.first.length() + 1 + sizeof(fCurrent.second);

	read();
}

bool CMergeIndexBuffer::iterator::equal(const iterator& inOther) const
{
	return this == &inOther or (fFile == inOther.fFile and fOffset == inOther.fOffset);
}

CMergeIndexBuffer::iterator::reference CMergeIndexBuffer::iterator::dereference() const
{
	return fCurrent;
}

void CMergeIndexBuffer::iterator::read()
{
	if (fOffset < fFile->Size())
	{
		fFile->Seek(fOffset, SEEK_SET);
		(*fFile) >> fCurrent.first >> fCurrent.second;
	}
	else
		fCurrent.first.clear();
}

// dang... the next function is nasty....
// First find out what indices we're going to create.
// Each part may contain a different set of indices

void CIndexer::MergeIndices(HStreamBase& outData, vector<CDatabank*>& inParts)
{
	vector<CDatabank*>::iterator p;
	vector<CMergeData> md;
	set<string> index_names;
	uint32 ix;

	fHeader->sig = kIndexSig;
	fHeader->entries = 0;
	
	fFile = &outData;
	
	for (p = inParts.begin(); p != inParts.end(); ++p)
	{
		CIndexer* index = (*p)->GetIndexer();

		fHeader->entries += index->fHeader->entries;
		
		CMergeData m;
		
		for (ix = 0; ix < index->fHeader->count; ++ix)
			m.info.push_back(index->fParts[ix]);
		m.count = index->fHeader->entries;

		md.push_back(m);
	}
	
	for (ix = 0; ix < md.size(); ++ix)
	{
		for (uint32 i = 0; i < md[ix].info.size(); ++i)
			index_names.insert(string(md[ix].info[i].name));
	}

	// now we know how many indices there are
	fHeader->count = index_names.size();
	
	for (ix = 0; ix < md.size(); ++ix)
	{
		// see if this part is missing some indices
		vector<string> names;
		copy(index_names.begin(), index_names.end(),
			back_insert_iterator<vector<string> >(names));
		
		vector<string>::iterator n;
		for (uint32 i = 0; i < md[ix].info.size(); ++i)
		{
			n = find(names.begin(), names.end(), string(md[ix].info[i].name));
			assert(n != names.end());
			if (n != names.end())
				names.erase(n);
		}
		
		for (n = names.begin(); n != names.end(); ++n)
		{
			SIndexPart info = { kIndexPartSig, 0 };
			(*n).copy(info.name, sizeof(info.name) - 1);
			md[ix].info.push_back(info);
		}
		
		sort(md[ix].info.begin(), md[ix].info.end(), CCompareIndexInfo());
	}

	// Now each CMergeData object should contain a similar info member
	// so we can safely iterate over the first info member and be sure
	// all the other parts have the same index at that location.
	
	delete[] fParts;
	fParts = new SIndexPart[fHeader->count];
	memset(fParts, 0, sizeof(SIndexPart) * fHeader->count);
	
	// Write out the initial index info table to allocate the diskspace
	outData << *fHeader;
	int64 offset = outData.Seek(0, SEEK_END);
	for (ix = 0; ix < fHeader->count; ++ix)
		outData << fParts[ix];
	
	for (ix = 0; ix < md[0].info.size(); ++ix)
	{
		fParts[ix] = md[0].info[ix];
		
		if (VERBOSE >= 1)
		{
			cout << "Merging index " << fParts[ix].name << "...";
			cout.flush();
		}

		uint32 i;
		int32 count = 0;
		auto_ptr<CJoinedIteratorBase> iter(CJoinedIteratorBase::Create(fParts[ix].kind));
		
		for (i = 0; i < md.size(); ++i)
		{
			md[i].indx = nil;
			
			if (md[i].info[ix].kind == 0)	// this index is not available in this part
				continue;
			
			if (fParts[ix].kind != md[i].info[ix].kind)
			{
				if (fParts[ix].kind == 0)
					fParts[ix].kind = md[i].info[ix].kind;
				else
					THROW(("Incompatible index types"));
			}

			md[i].data = &inParts[i]->GetDataFile();
			md[i].indx = new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
				*md[i].data, md[i].info[ix].tree_offset, md[i].info[ix].root);

			uint32 delta = 0;
			if (fParts[ix].kind == kValueIndex)
				delta = count;
			
			iter->Append(new CIteratorWrapper<CIndex>(*md[i].indx, delta), i);
			
			count += md[i].count;
		}

		auto_ptr<HStreamBase> bitFile(nil);
		HUrl url(fDb + ".indexbits");

		if (fParts[ix].kind != kValueIndex)
			bitFile.reset(new HTempFileStream(url));

		HAutoBuf<float> dwb(nil);
		float* dw = nil;
		if (fParts[ix].kind == kWeightedIndex)
		{
			dwb.reset(new float[fHeader->entries]);
			dw = dwb.get();
			
			memset(dw, 0, sizeof(float) * fHeader->entries);
		}

		fParts[ix].tree_offset = outData.Seek(0, SEEK_END);
		fParts[ix].root = 0;
		fParts[ix].bits_offset = 0;
		
//		list<pair<string,uint32> > indx;
		CMergeIndexBuffer indx(fDb);
		string s, lastS;
		
		fParts[ix].entries = 0;
		fParts[ix].sig = kIndexPartSig;
		fParts[ix].large_offsets = false;
		
		vector<pair<uint32,int64> > v;
		
		while (iter->Next(s, v))
		{
			if (s == lastS)
			{
				if (fParts[ix].kind == kValueIndex)
					THROW(("Attempt to enter duplicate key '%s' in index '%s'",
						s.c_str(), fParts[ix].name));
				continue;
			}

			lastS = s;

			++fParts[ix].entries;
			
			switch (fParts[ix].kind)
			{
				case kValueIndex:
					indx.push_back(make_pair(s, v[0].second));
					break;
				
				case kTextIndex:
				case kDateIndex:
				case kNumberIndex:
				{
					vector<uint32> docs;
					uint32 first = 0;
					
					for (i = 0; i < md.size(); ++i)
					{
						for (uint32 j = 0; j < v.size(); ++j)
						{
							if (v[j].first == i)
							{
								CDbDocIterator docIter(*md[i].data,
									md[i].info[ix].bits_offset + v[j].second, md[i].count);
							
								uint32 doc;
								while (docIter.Next(doc, false))
									docs.push_back(doc + first);
								
								break;
							}
						}

						first += md[i].count;
					}
					
					int64 offset = bitFile->Seek(0, SEEK_END);
					if (offset > numeric_limits<uint32>::max())
					{
						fParts[ix].sig = kIndexPartSigV2;
						fParts[ix].large_offsets = true;
					}

					COBitStream bits(*bitFile.get());
					CompressArray(bits, docs, fHeader->entries);
					
					indx.push_back(make_pair(s, offset));
					break;
				}
				
				case kWeightedIndex:
				{
					vector<pair<uint32,uint8> > docs;
					uint32 first = 0;
					
					for (i = 0; i < md.size(); ++i)
					{
						for (uint32 j = 0; j < v.size(); ++j)
						{
							if (v[j].first == i)
							{
								CDbDocWeightIterator docIter(*md[i].data,
									md[i].info[ix].bits_offset + v[j].second, md[i].count);
							
								uint32 doc;
								uint8 freq;

								float idf = docIter.GetIDFCorrectionFactor();

								while (docIter.Next(doc, freq, false))
								{
									docs.push_back(make_pair(doc + first, freq));

									float wdt = freq * idf;
									dw[doc + first] += wdt * wdt;
								}
								
								break;
							}
						}

						first += md[i].count;
					}
					
					int64 offset = bitFile->Seek(0, SEEK_END);
					if (offset > numeric_limits<uint32>::max())
					{
						fParts[ix].sig = kIndexPartSigV2;
						fParts[ix].large_offsets = true;
					}

					COBitStream bits(*bitFile.get());
					CompressArray(bits, docs, fHeader->entries);
					
					indx.push_back(make_pair(s, offset));
					break;
				}
			}
		}
		
		CIteratorWrapper<CMergeIndexBuffer> mapIter(indx);
		auto_ptr<CIndex> indxOnDisk(CIndex::CreateFromIterator(fParts[ix].kind,
			fParts[ix].large_offsets, mapIter, outData));
		fParts[ix].root = indxOnDisk->GetRoot();
		
		if (fParts[ix].kind != kValueIndex)
		{
			fParts[ix].bits_offset = outData.Seek(0, SEEK_END);
			
			char b[10240];
			int64 k = bitFile->Size();
			bitFile->Seek(0, SEEK_SET);
			
			while (k > 0)
			{
				int64 n = k;
				if (n > sizeof(b))
					n = sizeof(b);
				
				bitFile->Read(b, n);
				outData.Write(b, n);
				k -= n;
			}
		}
		
		// allocate disk space for the doc weight vector, if needed
		if (fParts[ix].kind == kWeightedIndex)
		{
			fParts[ix].weight_offset = outData.Seek(0, SEEK_END);
			
			for (uint32 d = 0; d < fHeader->entries; ++d)
				dw[d] = net_swapper::swap(sqrt(dw[d]));
			
			outData.Write(dw, sizeof(float) * fHeader->entries);
		}
		
		for (i = 0; i < md.size(); ++i)
		{
			delete md[i].indx;
			md[i].indx = nil;
		}

		if (bitFile.get() != nil)
			bitFile.reset(nil);

		if (VERBOSE >= 1)
			cout << " done (" << fParts[ix].entries << " entries)" << endl;
	}
	
	// Write out the final index info table
	outData.Seek(offset, SEEK_SET);
	for (ix = 0; ix < fHeader->count; ++ix)
		outData << fParts[ix];
}

/*
	Query support
*/

long CIndexer::GetIndexCount() const
{
	return fHeader->count;
}

void CIndexer::GetIndexInfo(uint32 inIndexNr, string& outCode,
						string& outType, uint32& outCount) const
{
	if (inIndexNr >= fHeader->count)
		THROW(("Index number out of range"));

	outCode = fParts[inIndexNr].name;
	outType.assign(reinterpret_cast<const char*>(&fParts[inIndexNr].kind), 4);
	outCount = fParts[inIndexNr].entries;
}

CDocIterator* CIndexer::GetImpForPattern(const string& inIndex,
	const string& inValue)
{
	string index = tolower(inIndex);
	
	auto_ptr<CDocIterator> result(nil);
	
	uint32 ix = 0;
	while (ix < fHeader->count and index != fParts[ix].name)
		++ix;

	if (ix == fHeader->count)
		THROW(("Index '%s' not found", inIndex.c_str()));

	auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
		*fFile, fParts[ix].tree_offset, fParts[ix].root));

	vector<uint32> values;
	indx->GetValuesForPattern(inValue, values);
	
	if (values.size())
	{
		vector<CDocIterator*> ixs;
		
		if (fParts[ix].kind == kValueIndex)
		{
			for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
				ixs.push_back(new CDocNrIterator(*i));
		}
		else if (fParts[ix].kind == kTextIndex or fParts[ix].kind == kDateIndex or fParts[ix].kind == kNumberIndex)
		{
			for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
				ixs.push_back(new CDbDocIterator(*fFile,
					fParts[ix].bits_offset + *i, fHeader->entries));
		}
		
		result.reset(CDocUnionIterator::Create(ixs));
	}

	return result.release();
}

CDocIterator* CIndexer::CreateDocIterator(const string& inIndex, const string& inKey,
	bool inKeyIsPattern, CQueryOperator inOperator)
{
	vector<CDocIterator*> iters;

	string index = tolower(inIndex);
	string key = tolower(inKey);

	for (uint32 ix = 0; ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name and index != "*")
			continue;

		if (inKeyIsPattern)
			iters.push_back(GetImpForPattern(fParts[ix].name, key));
		else if (inOperator != kOpContains)
		{
			if (inIndex == "*")
				THROW(("Cannot search with a relational operator on any index"));
			
			if (fParts[ix].kind == kTextIndex)
				THROW(("Cannot search with a relational operator on a fulltext index"));
			
			auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
				*fFile, fParts[ix].tree_offset, fParts[ix].root));

			vector<uint32> values;
			
			indx->GetValuesForOperator(inKey, inOperator, values);
			
			if (values.size())
			{
				if (fParts[ix].kind == kValueIndex)
				{
					for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
						iters.push_back(new CDocNrIterator(*i));
				}
				else// if (or fParts[ix].kind == kDateIndex or fParts[ix].kind == kNumberIndex)
				{
					for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
						iters.push_back(new CDbDocIterator(*fFile,
							fParts[ix].bits_offset + *i, fHeader->entries));
				}
			}
		}
		else
		{
			auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
				*fFile, fParts[ix].tree_offset, fParts[ix].root));
		
			int64 value;
			if (indx->GetValue(key, value))
			{
				if (fParts[ix].kind == kValueIndex)
					iters.push_back(new CDocNrIterator(value));
				else if (fParts[ix].kind == kTextIndex or fParts[ix].kind == kDateIndex or fParts[ix].kind == kNumberIndex)
					iters.push_back(new CDbDocIterator(*fFile,
						fParts[ix].bits_offset + value, fHeader->entries));
			}
		}
	}
	
	return CDocUnionIterator::Create(iters);
}

uint32 CIndexer::Count() const
{
	return fHeader->entries;
}

void CIndexer::PrintInfo()
{
	const char* sig = reinterpret_cast<const char*>(&fHeader->sig);
	
	cout << "Index Header ("
		 << sig[0] << sig[1] << sig[2] << sig[3] << ") " << fHeader->size << " bytes" << endl;
	cout << "  entries:          " << fHeader->entries << endl;
	cout << "  count:            " << fHeader->count << endl;
	cout << "  weight bit count: " << fHeader->weight_bit_count << endl;
	cout << endl;
	
	for (uint32 ix = 0; ix < fHeader->count; ++ix)
	{
		SIndexPart& p = fParts[ix];
		
		sig = reinterpret_cast<const char*>(&p.sig);
		
		cout << "Index Part " << ix << " ("
			 << sig[0] << sig[1] << sig[2] << sig[3] << ") " << fHeader->size << " bytes" << endl;

		cout << "  name:          " << p.name << endl;
		
		int64 nextOffset = fOffset + fSize;
		uint32 treeSize, bitsSize;
		
		if (ix < fHeader->count - 1)
		{
			if (fParts[ix + 1].bits_offset == 0)
				nextOffset = fParts[ix + 1].tree_offset;
			else
				nextOffset = min(fParts[ix + 1].tree_offset, fParts[ix + 1].bits_offset);
		}
		
		if (p.bits_offset > p.tree_offset)
		{
			treeSize = p.bits_offset - p.tree_offset;
			bitsSize = nextOffset - p.bits_offset;
		}
		else
		{
			bitsSize = p.tree_offset - p.bits_offset;
			treeSize = nextOffset - p.tree_offset;
		}
		
		sig = reinterpret_cast<const char*>(&p.kind);
		cout << "  kind:          " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
		cout << "  bits offset:   " << p.bits_offset << endl;
		if (p.kind != kValueIndex)
			cout << "  bits size:     " << bitsSize << endl;
		cout << "  tree offset:   " << p.tree_offset << endl;
		cout << "  tree size:     " << treeSize << endl;
		cout << "  root:          " << p.root << endl;
		cout << "  entries:       " << p.entries << endl;
		cout << "  weight offset: " << p.weight_offset << endl;
		
		cout << endl;

#if 0 //P_DEBUG
		if (p.kind == kWeightedIndex)
		{
			auto_ptr<CIndex> indx(new CIndex(p.kind, *fFile, p.tree_offset, p.root));
			
			for (CIndex::iterator v = indx->begin(); v != indx->end(); ++v)
			{
				cout << v->first << ": ";
				
				CDbDocWeightIterator iter(*fFile, p.bits_offset + v->second, fHeader->entries);
				
				uint32 k = iter.Count();

				uint32 d;
				uint8 r;
				
				while (iter.Next(d, r, false))
				{
					--k;
					cout << d << "-" << static_cast<uint32>(r) << " ";
				}
				
				cout << endl;

				assert(k == 0);
			}
		}
#endif
	}
}

class CIndexIteratorWrapper : public CIteratorWrapper<CIndex>
{
  public:
						CIndexIteratorWrapper(CIndex* inIndex, CIndex::iterator inCurrent)
							: CIteratorWrapper<CIndex>(*inIndex)
							, fIndex(inIndex)
						{
							fCurrent = inCurrent;
						}

  private:
	auto_ptr<CIndex>	fIndex;
};

CIteratorBase* CIndexer::GetIteratorForIndex(const string& inIndex)
{
	auto_ptr<CIndex> indx(GetIndex(inIndex));
	CIteratorWrapper<CIndex>* result = nil;

	if (indx.get())
	{
		result = new CIndexIteratorWrapper(indx.get(), indx->begin());
		indx.release();
	}
	
	return result;
}

CIteratorBase* CIndexer::GetIteratorForIndexAndKey(const string& inIndex, const string& inKey)
{
	auto_ptr<CIndex> indx(GetIndex(inIndex));
	CIteratorWrapper<CIndex>* result = nil;

	if (indx.get())
	{
		result = new CIndexIteratorWrapper(indx.get(), indx->find(inKey));
		indx.release();
	}
	
	return result;
}

CDocWeightArray CIndexer::GetDocWeights(const std::string& inIndex)
{
	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(index);

	uint32 ix;
	
	for (ix = 0; ix < fHeader->count; ++ix)
	{
		if (index == fParts[ix].name)
			break;
	}
	
	if (ix >= fHeader->count)
		THROW(("No document weight vector found for index %s", inIndex.c_str()));

	HFileStream* file = dynamic_cast<HFileStream*>(fFile);
	if (file == nil)
		THROW(("Stream should be a file in GetDocWeights"));

	CDocWeightArray arr(*file, fParts[ix].weight_offset, fHeader->entries);
	return arr;
}

CDbDocIteratorBase* CIndexer::GetDocWeightIterator(const string& inIndex, const string& inKey)
{
	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(index);

	CDbDocIteratorBase* result = nil;
	
	for (uint32 ix = 0; result == nil and ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name)
			continue;

		auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
			*fFile, fParts[ix].tree_offset, fParts[ix].root));

		int64 value;
		if (indx->GetValue(inKey, value))
			result = new CDbDocWeightIterator(*fFile, fParts[ix].bits_offset + value, fHeader->entries);

		break;
	}

	return result;
}

CIndex* CIndexer::GetIndex(const string& inIndex) const
{
	string index = tolower(inIndex);
	CIndex* result = nil;

	for (uint32 ix = 0; result == nil and ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name)
			continue;

		result = new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
			*fFile, fParts[ix].tree_offset, fParts[ix].root);
	}
	
	return result;
}

CFullTextIndex&	CIndexer::GetFullTextIndex()
{
	if (fFullTextIndex == nil)
	{
		HUrl url(fDb + ".fulltext_indx");
		fFullTextIndex = new CFullTextIndex(url);
	}

	return *fFullTextIndex;
}

void CIndexer::DumpIndex(const string& inIndex) const
{
	auto_ptr<CIndex> index(GetIndex(inIndex));

#if P_DEBUG
	index->Dump();
#else
	CIndex::iterator end = index->end();
	for (CIndex::iterator i = index->begin(); i != end; ++i)
		cout << i->first << '\t' << i->second << endl;
#endif
}

bool CIndexer::GetDocumentNr(const string& inDocumentID, uint32& outDocNr)
{
	bool result = true;
	auto_ptr<CIndex> index(GetIndex("id"));
	
	if (index.get())
	{
		if (index->GetKind() != kValueIndex)
			THROW(("ID index is of the wrong kind, should be a value index"));

		result = index->GetValue(inDocumentID, outDocNr);
	}
	else
	{
		stringstream s(inDocumentID);
		s >> outDocNr;
	}
	
	return result;
}

class CWeightAccumulator
{
  public:
					CWeightAccumulator(HStreamBase& inFile,
							float inWeights[], uint32 inDocCount)
						: fFile(inFile)
						, fWeights(inWeights)
						, fDocCount(inDocCount)
					{
					}
	
	void			Visit(const string& inKey, int64 inValue);

  private:
	HStreamBase&	fFile;
	float*			fWeights;
	uint32			fDocCount;
};

void CWeightAccumulator::Visit(const string& inKey, int64 inValue)
{
	CDbDocWeightIterator iter(fFile, inValue, fDocCount);
	
	float idf = iter.GetIDFCorrectionFactor();
	
	uint32 doc;
	uint8 freq;

	while (iter.Next(doc, freq, false))
	{
		float wdt = freq * idf;
		fWeights[doc] += wdt * wdt;
	}
}

void CIndexer::RecalculateDocumentWeights(const std::string& inIndex)
{
	if (VERBOSE > 0)
	{
		cout << "Recalculating document weights for index " << inIndex << "... ";
		cout.flush();
	}
	
	string index = inIndex;
	if (index != kAllTextIndexName)	
		index = tolower(index);
	
	auto_ptr<CIndex> indx;

	uint32 ix;
	for (ix = 0; ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name)
			continue;

		indx.reset(new CIndex(fParts[ix].kind, fParts[ix].large_offsets,
			*fFile, fParts[ix].tree_offset, fParts[ix].root));
		break;
	}

	if (indx.get() == nil)
		THROW(("Index %s not found!", inIndex.c_str()));
	
	uint32 docCount = fHeader->entries;
	
	HAutoBuf<float> dwb(new float[docCount]);
	float* dw = dwb.get();
	
	memset(dw, 0, docCount * sizeof(float));
	
	int64 bitsOffset = fParts[ix].bits_offset;
	
	HStreamView bits(*fFile, bitsOffset, fFile->Size() - bitsOffset);
	CWeightAccumulator accu(bits, dw, docCount);
	indx->Visit(&accu, &CWeightAccumulator::Visit);
	
	for (uint32 d = 0; d < docCount; ++d)
	{
		if (dw[d] != 0)
			dw[d] = net_swapper::swap(sqrt(dw[d]));
	}
	
	fFile->PWrite(dw, docCount * sizeof(float), fParts[ix].weight_offset);
	
	if (VERBOSE > 0)
		cout << "done" << endl;
}

void CIndexer::FixupDocWeights()
{
	for (uint32 ix = 0; ix < fHeader->count; ++ix)
	{
		if (fParts[ix].kind == kWeightedIndex)
			RecalculateDocumentWeights(fParts[ix].name);
	}
}

uint32 CIndexer::GetMaxWeight() const
{
	return ((1 << fHeader->weight_bit_count) - 1);
}
