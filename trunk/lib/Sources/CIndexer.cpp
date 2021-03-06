/*	$Id: CIndexer.cpp 331 2007-02-12 07:44:10Z hekkel $
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

#ifndef HAVE_TR1
#if P_GNU
#warning("HAVE_TR1 is not defined, reverting to use __gnu_cxx::hash_set (add HAVE_TR1=1 to make.config if you want to use tr1)")
#else
#pragma warning("HAVE_TR1 is not defined, reverting to use std::set")
#endif
#define	HAVE_TR1 0
#endif

#include <list>
#include <set>
#include <iostream>
#include <limits>
#include <ctime>
#include <sstream>
#if HAVE_TR1
#include <tr1/unordered_set>
#elif P_GNU
#include <ext/hash_set>
#endif
#include <iterator>
#include <boost/functional/hash/hash.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <numeric>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include "HFile.h"
#include "HUtils.h"
#include "HError.h"
#include "HMutex.h"
#include "HBuffer.h"

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
	uint32			array_compression_kind;
};

const uint32
	kSIndexHeaderSize = 5 * sizeof(uint32) + sizeof(int64);

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
	int64			weight_offset;	// new since V1
	int64			tree_size;		// these are new since V2
	int64			bits_size;
	
	uint32			flags;			// new for version V4
	
	// members that are implictly part of the above
	CIndexVersion	index_version;
};

const uint32
	kContainsIDL	= (1 << 0);

const uint32
	kSIndexPartSizeV4 = 6 * sizeof(uint32) + 5 * sizeof(int64) + 16,	// V4
	kSIndexPartSizeV2 = 5 * sizeof(uint32) + 5 * sizeof(int64) + 16,	// V2
	kSIndexPartSizeV1 = 5 * sizeof(uint32) + 3 * sizeof(int64) + 16,	// V1
	kSIndexPartSizeV0 = 5 * sizeof(uint32) + 2 * sizeof(int64) + 16,	// V0
	
	kSIndexPartSize = kSIndexPartSizeV4;								// current

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
	data.Write(&inStruct.array_compression_kind, sizeof(inStruct.array_compression_kind));
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SIndexHeader& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	data >> inStruct.size >> inStruct.entries >> inStruct.count;
	
	if (inStruct.size >= kSIndexHeaderSize - sizeof(uint32))
		data >> inStruct.weight_bit_count;
	else
		inStruct.weight_bit_count = 6;

	if (inStruct.size >= kSIndexHeaderSize)
	{
		data.Read(&inStruct.array_compression_kind, sizeof(inStruct.array_compression_kind));
		
		// stupid me...
		
		if (inStruct.array_compression_kind == FOUR_CHAR_INLINE('1les'))
			inStruct.array_compression_kind = kAC_SelectorCodeV1;
		else if (inStruct.array_compression_kind == FOUR_CHAR_INLINE('olog'))
			inStruct.array_compression_kind = kAC_GolombCode;
	}
	else
		inStruct.array_compression_kind = kAC_GolombCode;
	
	return inData;
}

HStreamBase& operator<<(HStreamBase& inData, const SIndexPart& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	if (inStruct.sig != 0)
		assert((inStruct.sig == kIndexPartSig and inStruct.index_version == kCIndexVersionV1) or
			   (inStruct.sig == kIndexPartSigV2 and inStruct.index_version == kCIndexVersionV2));

	data.Write(&inStruct.sig, sizeof(inStruct.sig));
	data << kSIndexPartSize;
	data.Write(inStruct.name, sizeof(inStruct.name));
	data.Write(&inStruct.kind, sizeof(inStruct.kind));

	data << inStruct.bits_offset << inStruct.tree_offset
		 << inStruct.root << inStruct.entries
		 << inStruct.weight_offset
		 << inStruct.tree_size << inStruct.bits_size
		 << inStruct.flags;
	
	return inData;
}

HStreamBase& operator>>(HStreamBase& inData, SIndexPart& inStruct)
{
	HSwapStream<net_swapper> data(inData);
	
	data.Read(&inStruct.sig, sizeof(inStruct.sig));
	data >> inStruct.size;

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
	{
		if (inStruct.size == kSIndexPartSizeV0)
			inStruct.index_version = kCIndexVersionV0;
		else
			inStruct.index_version = kCIndexVersionV1;
	}
	else if (inStruct.sig == kIndexPartSigV2)
		inStruct.index_version = kCIndexVersionV2;
	else
		THROW(("Incompatible index %4.4s", &inStruct.sig));
	
	data.Read(inStruct.name, sizeof(inStruct.name));
	data.Read(&inStruct.kind, sizeof(inStruct.kind));
	data >> inStruct.bits_offset >> inStruct.tree_offset
		 >> inStruct.root >> inStruct.entries;
	
	inStruct.weight_offset = 0;
	
	if (inStruct.size >= kSIndexPartSizeV1)
		data >> inStruct.weight_offset;

	inStruct.tree_size = 0;
	inStruct.bits_size = 0;

	if (inStruct.size >= kSIndexPartSizeV2)
		data >> inStruct.tree_size >> inStruct.bits_size;

	inStruct.flags = 0;

	if (inStruct.size >= kSIndexPartSizeV4)
		data >> inStruct.flags;

	return inData;
}

class CMergeWeightedDocInfo
{
  public:
					CMergeWeightedDocInfo(uint32 inDocDelta,
							CDbDocIteratorBase* inIterator)
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
	CDbDocIteratorBase*	fIter;
	uint32			fDocNr;
	uint8			fWeight;
};

// index

const uint32
//	kRunBlockSize = 0x100000,		// 1 Mb
	kBufferEntryCount = 2000000;

typedef vector<uint32>		DocLoc;

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
					CFullTextIndex(const HUrl& inScratch, uint32 inWeightBitCount);
	virtual			~CFullTextIndex();
	
	void			ReleaseBuffer();
	
	void			SetUsesInDocLocation(uint32 inIndexNr)						{ fDocLocationIxMap |= (1 << inIndexNr); }
	bool			UsesInDocLocation(uint32 inIndexNr) const					{ return (fDocLocationIxMap & (1 << inIndexNr)) != 0; }

	// two versions for AddWord, one works with lexicon's ID's
//	void			AddWord(uint8 inIndex, const string& inWord, uint8 inFrequency);
	void			AddWord(uint8 inIndex, uint32 inWord, uint8 inFrequency);
	
	void			Stop();				// called to increase fDocWordLocation
	
	void			FlushDoc(uint32 inDocNr);
	
	void			Merge();
	
	// iterator functions
	// to iterate over the run info
	class CRunEntryIterator
	{
	  public:
						CRunEntryIterator(CFullTextIndex& inIndex);
						~CRunEntryIterator();
		
		bool			Next(uint32& outDoc, uint32& outTerm, uint32& outIx, uint8& outWeight);
		
		CIBitStream&	Bits()				{ return fLastInfo->bits; }
		
		int64			Count() const		{ return fEntryCount; }
	
	  private:
		CFullTextIndex&	fIndex;
		MergeInfo**		fMergeInfo;
		MergeInfo*		fLastInfo;
		uint32			fRunCount;
		int64			fEntryCount;
	};

	friend class CRunEntryIterator;

	uint32			GetWeightBitCount() const		{ return fWeightBitCount; }
	
  private:
	
	struct DocWord
	{
		uint32		word;
		uint32		index;
		uint32		freq;
		DocLoc		loc;
		
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
		DocLoc		loc;	// 'in document' location
		
		bool		operator<(const BufferEntry& inOther) const
						{ return term < inOther.term or (term == inOther.term and doc < inOther.doc); }
	};
	
//	typedef BufferEntry	BufferEntryArray[kBufferEntryCount];
//	typedef BufferEntryArray*	BufferEntryArrayPtr;
	struct FlushBufferEntryData
	{
		BufferEntry*	buffer;
		uint32			count;
	};

	typedef HBuffer<FlushBufferEntryData,10>	FlushBufferEntryDataBuffer;
	
	void			FlushRun();

	void			FlushRunThread();

	struct RunInfo
	{
		int64		offset;
		uint32		count;
	};
	typedef vector<RunInfo>	RunInfoArray;
	
	struct MergeInfo
	{
					MergeInfo(CFullTextIndex& inIndex, HStreamBase& inFile,
							int64 inOffset, uint32 inCount, uint32 inWeightBitCount)
						: cnt(inCount)
						, bits(inFile, inOffset)
						, ftix(inIndex)
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
		CFullTextIndex&
					ftix;
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

#if HAVE_TR1
	typedef tr1::unordered_set<uint32, boost::hash<uint32> >	CHashedSet;
#elif P_GNU
	typedef __gnu_cxx::hash_set<uint32>							CHashedSet;
#else
	typedef set<uint32>											CHashedSet;
#endif

	uint32			fWeightBitCount;
	DocWords		fDocWords;
	uint32			fFTIndexCnt;
	uint32			fDocLocationIxMap;
	uint32			fDocWordLocation;
	BufferEntry*	buffer;
	uint32			buffer_ix;
	BufferEntry*	flushed_buffer;
	uint32			flushed_buffer_ix;
	RunInfoArray	fRunInfo;
	HUrl			fScratchUrl;
	HStreamBase*	fScratch;
	
	// pipe line for flushing runs of BufferEntry
	FlushBufferEntryDataBuffer
					fFlushRunBuffer;
	boost::thread*	fFlushRunThread;
};

CFullTextIndex::CFullTextIndex(const HUrl& inScratchUrl, uint32 inWeightBitCount)
	: fWeightBitCount(inWeightBitCount)
	, fFTIndexCnt(0)
	, fDocLocationIxMap(0)
	, fDocWordLocation(0)
	, buffer(new BufferEntry[kBufferEntryCount])
	, buffer_ix(0)
	, flushed_buffer(nil)
	, flushed_buffer_ix(0)
	, fScratchUrl(inScratchUrl)
	, fScratch(new HTempFileStream(fScratchUrl))
	, fFlushRunThread(nil)
{
}

CFullTextIndex::~CFullTextIndex()
{
	if (fFlushRunThread != nil)
	{
		fFlushRunThread->join();
		delete fFlushRunThread;
	}
	
	delete[] buffer;
	delete fScratch;
}

void CFullTextIndex::AddWord(uint8 inIndex, uint32 inWord, uint8 inFrequency)
{
	++fDocWordLocation;	// always increment, no matter if we do not add the word
	if (fDocWordLocation >= kMaxInDocumentLocation)	// cycle...
		fDocWordLocation = 1;
	
	if (inIndex > kMaxIndexNr)
		THROW(("Too many full text indices"));
	
	if (fFTIndexCnt < uint32(inIndex + 1))
		fFTIndexCnt = inIndex + 1;

	if (inFrequency > 0)
	{
		DocWord w = { inWord, inIndex, inFrequency };
		
		DocWords::iterator i = fDocWords.find(w);
		if (i != fDocWords.end())
			const_cast<DocWord&>(*i).freq += inFrequency;
		else
			i = fDocWords.insert(w).first;
		
		if (UsesInDocLocation(inIndex) and fDocWordLocation < kMaxInDocumentLocation)
		{
			DocWord& dw = const_cast<DocWord&>(*i);
			dw.loc.push_back(fDocWordLocation);
		}
	}
}

void CFullTextIndex::Stop()
{
	++fDocWordLocation;
	if (fDocWordLocation >= kMaxInDocumentLocation)	// cycle...
		fDocWordLocation = 1;
}

void CFullTextIndex::FlushDoc(uint32 inDoc)
{
	if (buffer_ix + fDocWords.size() >= kBufferEntryCount)
		FlushRun();

	// normalize the frequencies.
	
	uint32 maxFreq = 1;
	uint32 kMaxWeight = (1 << fWeightBitCount) - 1;
	
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

		buffer[buffer_ix].weight = (w->freq * kMaxWeight) / maxFreq;
		if (buffer[buffer_ix].weight < 1)
			buffer[buffer_ix].weight = 1;
	
		assert(buffer[buffer_ix].weight <= kMaxWeight);
		assert(buffer[buffer_ix].weight > 0);
		
		if (UsesInDocLocation(w->index))
		{
			DocWord& dw = const_cast<DocWord&>(*w);
			swap(buffer[buffer_ix].loc, dw.loc);
		}

		++buffer_ix;
	}
	
	fDocWords.clear();
	fDocWordLocation = 0;
}

void CFullTextIndex::FlushRun()
{
	if (fFlushRunThread == nil)
		fFlushRunThread = new boost::thread(boost::bind(&CFullTextIndex::FlushRunThread, this));

	FlushBufferEntryData next = { buffer, buffer_ix };

	fFlushRunBuffer.Put(next);
	
	buffer = new BufferEntry[kBufferEntryCount];
	buffer_ix = 0;
}

void CFullTextIndex::FlushRunThread()
{
	for (;;)
	{
		FlushBufferEntryData next = fFlushRunBuffer.Get();
		
		if (next.buffer == nil)
			break;

		uint32 firstDoc = next.buffer[0].doc;	// the first doc in this run
	
		sort(next.buffer, next.buffer + next.count);
		
		RunInfo ri;
		ri.offset = fScratch->Seek(0, SEEK_END);
		ri.count = next.count;
		fRunInfo.push_back(ri);
		
		COBitStream bits(*fScratch);
	
		uint32 t = 0;
		uint32 d = firstDoc;	// the first doc in this run
		
		WriteGamma(bits, d + 1);
	
		for (uint32 i = 0; i < next.count; ++i)
		{
			if (next.buffer[i].term > t)
				d = firstDoc;
	
			WriteUnary(bits, (next.buffer[i].term - t) + 1);
			WriteGamma(bits, (next.buffer[i].doc - d) + 1);
			WriteGamma(bits, next.buffer[i].ix + 1);
	
			assert(next.buffer[i].weight > 0);
			assert(next.buffer[i].weight <= ((1 << fWeightBitCount) - 1));
	
			WriteBinary(bits, next.buffer[i].weight, fWeightBitCount);
			
			if (UsesInDocLocation(next.buffer[i].ix))
				CompressArray(bits, next.buffer[i].loc, kMaxInDocumentLocation);
			
			t = next.buffer[i].term;
			d = next.buffer[i].doc;
		}
		
		bits.sync();
	
		delete[] next.buffer;
	}
}

void CFullTextIndex::ReleaseBuffer()
{
	FlushRun();
	
	FlushBufferEntryData next = { nil, 0 };
	fFlushRunBuffer.Put(next);
		
	fFlushRunThread->join();
	
	buffer = nil;
}

CFullTextIndex::CRunEntryIterator::CRunEntryIterator(CFullTextIndex& inIndex)
	: fIndex(inIndex)
	, fLastInfo(nil)
{
	fRunCount = fIndex.fRunInfo.size();
	fMergeInfo = new MergeInfo*[fRunCount];

	fEntryCount = 0;
	for (RunInfoArray::iterator ri = fIndex.fRunInfo.begin(); ri != fIndex.fRunInfo.end(); ++ri)
	{
		MergeInfo* mi = new MergeInfo(inIndex, *fIndex.fScratch,
			ri->offset, ri->count, fIndex.GetWeightBitCount());
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

bool CFullTextIndex::CRunEntryIterator::Next(uint32& outDoc, uint32& outTerm,
	uint32& outIx, uint8& outWeight)
{
	bool result = false;

	if (fLastInfo != nil)
	{
		if (fLastInfo->cnt != 0)
		{
			fLastInfo->Next();
			push_heap(fMergeInfo, fMergeInfo + fRunCount, CompareMergeInfo());
		}
		else
		{
			delete fLastInfo;
			--fRunCount;
			fMergeInfo[fRunCount] = nil;
		}

		fLastInfo = nil;
	}

	if (fRunCount > 0)
	{
		if (fRunCount > 1)
			pop_heap(fMergeInfo, fMergeInfo + fRunCount, CompareMergeInfo());
		
		fLastInfo = fMergeInfo[fRunCount - 1];
		
		outDoc = fLastInfo->doc;
		outTerm = fLastInfo->term;
		outIx = fLastInfo->ix;
		outWeight = fLastInfo->weight;

		result = true;
		--fEntryCount;
	}
	
	return result;
}

// --------------------------------------------------------------------
//
//

class CFullTextIterator : public CIteratorBase
{
  public:
						CFullTextIterator(
							CLexicon&						inLexicon,
							vector<pair<uint32,int64> >&	inData);

	virtual bool		Next(string& outString, int64& outValue);

  private:
	CLexicon&								fLexicon;
	vector<pair<uint32,int64> >::iterator	fCurrent, fEnd;
};

CFullTextIterator::CFullTextIterator(
	CLexicon&						inLexicon,
	vector<pair<uint32,int64> >&	inData)
	: fLexicon(inLexicon)
	, fCurrent(inData.begin())
	, fEnd(inData.end())
{
}

bool CFullTextIterator::Next(
	string&		outKey,
	int64&		outValue)
{
	bool result = false;
	if (fCurrent != fEnd)
	{
		result = true;

		outKey = fLexicon.GetString(fCurrent->first);
		outValue = fCurrent->second;
		
		++fCurrent;
	}
	return result;
}

// --------------------------------------------------------------------
// 
// 	classes for the various kinds of indices
// 

// CIndexBase is the base class for the various index types, these
// classes are only used for building indices.

class CIndexBase : public CLexCompare
{
  public:
					CIndexBase(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch,
						CIndexKind			inKind);

	virtual 		~CIndexBase();
	
	void			SetIsUpdate(bool isUpdate)
											{ fIsUpdate = isUpdate; }
	
	bool			UsesInDocLocation() const	{ return fUsesIDL; }
	void			SetUsesInDocLocation(bool inUseIDL);

//	void			AddWord(const string& inWord, uint32 inFrequency = 1);
	void			AddWord(const uint32 inWord, uint32 inFrequency = 1);
	
	virtual void	AddDocTerm(uint32 inDoc, uint8 inFrequency, CIBitStream& inDocLoc);
	virtual void	FlushTerm(uint32 inTerm, uint32 inDocCount);
	
	virtual bool	Write(
						HStreamBase&		inDataFile,
						uint32				inDocCount,
						SIndexPart&			outInfo);

					// NOTE: empty is overloaded (i.e. used for two purposes)
	bool			Empty() const				{ return fEmpty; }
	void			SetEmpty(bool inEmpty)		{ fEmpty = inEmpty; }
	uint16			GetIxNr() const				{ return fIndexNr; }
	string			GetName() const				{ return fName; }

	virtual int		Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const;

  protected:
	string			fName;
  	uint32			fKind;

	CFullTextIndex&	fFullTextIndex;
	CLexicon&		fLexicon;
	uint16			fIndexNr;
	bool			fEmpty;
	bool			fIsUpdate;
	bool			fUsesIDL;
	uint32			fWeightBitCount;
	
	// data for the second pass
	uint32			fLastDoc;
	COBitStream*	fBits;
	COBitStream*	fIDLBits;
	uint32			fDocCount;
	
	HUrl			fBitUrl;
	HStreamBase*	fBitFile;
	HMemoryStream	fScratch;
	HMemoryStream	fIDLScratch;
	
	COBitStream*	fRawIndex;
	uint32			fRawIndexCnt;
	uint32			fLastTermNr;
	int64			fLastOffset;
};

struct SortLex
{
			SortLex(
				CLexicon& 		inLexicon,
				CIndexBase*		inIndexBase)
				: fLexicon(inLexicon)
				, fBase(inIndexBase) {}
	
	inline
	bool	operator()(const pair<uint32,uint64>& inA, const pair<uint32,uint64>& inB) const
			{
				return fLexicon.Compare(inA.first, inB.first, *fBase) < 0;
			}
	
	CLexicon&		fLexicon;
	CIndexBase*		fBase;
};

CIndexBase::CIndexBase(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch,
	CIndexKind			inKind)
	: fName(inName)
	, fKind(inKind)
	, fFullTextIndex(inFullTextIndex)
	, fLexicon(inLexicon)
	, fIndexNr(inIndexNr)
	, fEmpty(true)
	, fIsUpdate(false)
	, fUsesIDL(false)
	, fWeightBitCount(0)
	, fLastDoc(0)
	, fBits(nil)
	, fIDLBits(nil)
	, fDocCount(0)
	, fBitUrl(inScratch)
	, fRawIndex(nil)
	, fRawIndexCnt(0)
	, fLastTermNr(0)
	, fLastOffset(0)
{
	fBitFile = new HTempFileStream(fBitUrl);
}

CIndexBase::~CIndexBase()
{
	delete fBits;
	delete fRawIndex;
	delete fBitFile;
	delete fIDLBits;
}

void CIndexBase::SetUsesInDocLocation(bool inUseIDL)
{
	if (inUseIDL)
	{
		fFullTextIndex.SetUsesInDocLocation(fIndexNr);
		fUsesIDL = true;
	}
}

int CIndexBase::Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const
{
	CIndexTraits<kTextIndex> comp;
	return comp.Compare(inA, inLengthA, inB, inLengthB);
}

//void CIndexBase::AddWord(const string& inWord, uint32 inFrequency)
//{
//	fFullTextIndex.AddWord(fIndexNr, inWord, inFrequency);
//	fEmpty = false;
//}

void CIndexBase::AddWord(const uint32 inWord, uint32 inFrequency)
{
	fFullTextIndex.AddWord(fIndexNr, inWord, inFrequency);
	fEmpty = false;
}

void CIndexBase::AddDocTerm(uint32 inDoc, uint8 inFrequency, CIBitStream& inDocLoc)
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
	
	if (fUsesIDL)
	{
		if (fIDLBits == nil)
			fIDLBits = new COBitStream(fIDLScratch);

		CopyArray<uint32,kAC_SelectorCodeV2>(inDocLoc, *fIDLBits, kMaxInDocumentLocation);
	}
	
	fLastDoc = inDoc;
	++fDocCount;
	fEmpty = false;
}

class CIDLInterleaver : public CValuePairCompression::CInterleavedDataWriter
{
  public:
					CIDLInterleaver(HStreamBase& inData);
	virtual void	WriteData(COBitStream& inBits);

  private:
	CIBitStream		fIDLBits;
};

CIDLInterleaver::CIDLInterleaver(HStreamBase& inData)
	: fIDLBits(inData, 0)
{
}

void CIDLInterleaver::WriteData(COBitStream& inBits)
{
	CopyArray<uint32,kAC_SelectorCodeV2>(fIDLBits, inBits, kMaxInDocumentLocation);
}

void CIndexBase::FlushTerm(uint32 inTerm, uint32 inDocCount)
{
	if (fDocCount > 0 and fBits != nil)
	{
		// flush the raw index bits
		fBits->sync();
		
		int64 offset = fBitFile->Seek(0, SEEK_END);
//		if (offset > numeric_limits<uint32>::max())
//			THROW(("Resulting full text index is too large"));
		
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
			auto_ptr<CIDLInterleaver> interleaver;
			
			if (fUsesIDL)
			{
				fIDLBits->sync();
				interleaver.reset(new CIDLInterleaver(fIDLScratch));
			}
			
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

			CompressArray(docBits, docs, inDocCount, interleaver.get());
		}

		docBits.sync();
	}

	delete fBits;
	fBits = nil;
	fDocCount = 0;
	fLastDoc = 0;
	fScratch.Truncate(0);
	
	if (fIDLBits != nil)
	{
		delete fIDLBits;
		fIDLBits = nil;
		fIDLScratch.Truncate(0);
	}
	
	fEmpty = true;
}

bool CIndexBase::Write(
	HStreamBase&		inDataFile,
	uint32				/*inDocCount*/,
	SIndexPart&			outInfo)
{
	if (fRawIndex == nil)	// shortcut in case we didn't collect any data
		return false;
	
	if (VERBOSE >= 1)
	{
		cout << endl << "Writing index '" << fName << "' nr: " << fIndexNr << "... ";
		cout.flush();
	}

	outInfo.sig = kIndexPartSig;
	fName.copy(outInfo.name, 15);
	outInfo.kind = fKind;

	outInfo.index_version = kCIndexVersionV1;
	if (fBitFile->Size() >= numeric_limits<uint32>::max())
	{
		outInfo.sig = kIndexPartSigV2;
		outInfo.index_version = kCIndexVersionV2;
	}

	vector<pair<uint32,int64> > lexicon;
	lexicon.reserve(fRawIndexCnt);

	// in the ideal case we store the bits in the same order as their
	// corresponding key in the BTree. That speeds up merging later on
	// considerably.
	// This smells a bit like hacking...
	// In case the bit sections is less than 4 Gb (32 bits wide) we
	// store the length of the bit vector in the upper 32 bits of the second
	// field of the pair in lexicon. 
	
	HAutoBuf<char> sortedBitBuffer(nil);
	vector<int64> offsets;
	
	try
	{
		if (fBitFile->Size() < numeric_limits<uint32>::max())
		{
			sortedBitBuffer.reset(new char[fBitFile->Size()]);
			offsets.reserve(fRawIndexCnt);
		}
	}
	catch (...)
	{
		sortedBitBuffer.reset(nil);
	}
	
	fRawIndex->sync();
	CIBitStream bits(fRawIndex->peek(), fRawIndex->size());
	
	uint32 term;
	int64 offset;
	term = ReadGamma(bits) - 1;
	offset = ReadGamma(bits) - 1;
	
	lexicon.push_back(make_pair(term, offset));
	--fRawIndexCnt;
	
	SortLex sortLex(fLexicon, this);
	
	while (fRawIndexCnt-- > 0)
	{
		term += ReadGamma(bits);
		
		int64 length = ReadGamma(bits);
		
		if (sortedBitBuffer.get() != nil)
			lexicon.back().second |= (length << 32);
		
		// strange location, I agree
		push_heap(lexicon.begin(), lexicon.end(), sortLex);

		offset += length;
		lexicon.push_back(make_pair(term, offset));
	}
	
	if (sortedBitBuffer.get() != nil)
		lexicon.back().second |= ((fBitFile->Size() - offset) << 32);

	push_heap(lexicon.begin(), lexicon.end(), sortLex);
	
	sort_heap(lexicon.begin(), lexicon.end(), sortLex);

	// copy the bits to the data file
	outInfo.bits_offset = inDataFile.Seek(0, SEEK_END);
	outInfo.bits_size = fBitFile->Size();
	
	if (sortedBitBuffer.get() != nil)
	{
		fBitFile->PRead(sortedBitBuffer.get(), outInfo.bits_size, 0);
		
		int64 newOffset = 0;
		
		for (vector<pair<uint32,int64> >::iterator l = lexicon.begin(); l != lexicon.end(); ++l)
		{
			uint32 length = static_cast<uint32>(l->second >> 32);
			uint32 offset = static_cast<uint32>(l->second);

			(void)inDataFile.Write(sortedBitBuffer.get() + offset, length);

			l->second = newOffset;
			newOffset += length;
		}
		
		assert(newOffset == outInfo.bits_size);
		
		sortedBitBuffer.release();
	}
	else
	{
		if (VERBOSE)
			cout << "Writing unsorted bits ";
		
		fBitFile->CopyTo(inDataFile, fBitFile->Size(), 0);
	}

	// construct the optimized b-tree

	CFullTextIterator iter(fLexicon, lexicon);
	auto_ptr<CIndex> indx(CIndex::CreateFromIterator(fKind, outInfo.index_version, iter, inDataFile));

	outInfo.tree_offset = indx->GetOffset();
	outInfo.tree_size = inDataFile.Seek(0, SEEK_END) - outInfo.tree_offset;
	outInfo.root = indx->GetRoot();
	outInfo.entries = lexicon.size();
	
	outInfo.flags = 0;
	if (fUsesIDL)
		outInfo.flags |= kContainsIDL;
	
	if (VERBOSE >= 1)
		cout << "done" << endl << "Wrote " << lexicon.size() << " entries." << endl;
	
	return true;
}

// CValueIndex, can only store one value per document and so
// it should be unique

class CValueIndex : public CIndexBase
{
  public:
					CValueIndex(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch);
	
	virtual void	AddDocTerm(uint32 inDoc, uint8 inFrequency, CIBitStream& inDocLoc);
	virtual void	FlushTerm(uint32 inTerm, uint32 inDocCount);

	virtual bool	Write(
						HStreamBase&		inDataFile,
						uint32				inDocCount,
						SIndexPart&			outInfo);
	
  private:
	vector<uint32>	fDocs;
	vector<pair<uint32,int64> >
					fIndex;
};

CValueIndex::CValueIndex(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch)
	: CIndexBase(inFullTextIndex, inLexicon, inName, inIndexNr, inScratch, kValueIndex)
{
}
	
void CValueIndex::AddDocTerm(uint32 inDoc, uint8 inFrequency, CIBitStream& inDocLoc)
{
	fDocs.push_back(inDoc);
	fEmpty = false;
}

void CValueIndex::FlushTerm(uint32 inTerm, uint32 inDocCount)
{
	if (fDocs.size() != 1 and not fIsUpdate)
	{
		string term = fLexicon.GetString(inTerm);

		cerr << endl
			 << "Term " << term << " is not unique for index "
			 << fName << ", it appears in document: ";

		for (vector<uint32>::iterator d = fDocs.begin(); d != fDocs.end(); ++d)
			cerr << *d << " ";

		cerr << endl;
	}
	
	if (fDocs.size() > 0)
		fIndex.push_back(make_pair(inTerm, fDocs.back()));
	
	fDocs.clear();
	fEmpty = true;
}

bool CValueIndex::Write(
	HStreamBase&		inDataFile,
	uint32				/*inDocCount*/,
	SIndexPart&			outInfo)
{
	if (fIndex.size() == 0)
		return false;
	
	if (VERBOSE >= 1)
	{
		cout << endl << "Writing index '" << fName << "'... ";
		cout.flush();
	}

	outInfo.sig = kIndexPartSig;
	outInfo.index_version = kCIndexVersionV1;
	fName.copy(outInfo.name, 15);
	outInfo.kind = fKind;

	// construct the optimized b-tree
	sort(fIndex.begin(), fIndex.end(), SortLex(fLexicon, this));

	CFullTextIterator iter(fLexicon, fIndex);
	auto_ptr<CIndex> indx(CIndex::CreateFromIterator(fKind, outInfo.index_version, iter, inDataFile));

	outInfo.tree_offset = indx->GetOffset();
	outInfo.tree_size = inDataFile.Seek(0, SEEK_END) - outInfo.tree_offset;
	outInfo.root = indx->GetRoot();
	outInfo.entries = fIndex.size();

	if (VERBOSE >= 1)
		cout << "done" << endl << "Wrote " << outInfo.entries << " entries." << endl;
	
	return true;
}

class CTextIndex : public CIndexBase
{
  public:
					CTextIndex(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch);
};

CTextIndex::CTextIndex(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch)
	: CIndexBase(inFullTextIndex, inLexicon, inName, inIndexNr, inScratch, kTextIndex)
{
}

class CWeightedWordIndex : public CIndexBase
{
  public:
					CWeightedWordIndex(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch);

	virtual bool	Write(
						HStreamBase&		inDataFile,
						uint32				inDocCount,
						SIndexPart&			outInfo);
};

CWeightedWordIndex::CWeightedWordIndex(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch)
	: CIndexBase(inFullTextIndex, inLexicon, inName, inIndexNr, inScratch, kWeightedIndex)
{
	fWeightBitCount = inFullTextIndex.GetWeightBitCount();
}

bool CWeightedWordIndex::Write(
	HStreamBase&		inDataFile,
	uint32				inDocCount,
	SIndexPart&			outInfo)
{
	if (not CIndexBase::Write(inDataFile, inDocCount, outInfo))
		return false;
	
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
	
	return true;
}

class CDateIndex : public CIndexBase
{
  public:
					CDateIndex(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch);
};

CDateIndex::CDateIndex(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch)
	: CIndexBase(inFullTextIndex, inLexicon, inName, inIndexNr, inScratch, kDateIndex)
{
}

class CNumberIndex : public CIndexBase
{
  public:
					CNumberIndex(
						CFullTextIndex&		inFullTextIndex,
						CLexicon&			inLexicon,
						const string&		inName,
						uint16				inIndexNr,
						const HUrl&			inScratch);

	virtual int		Compare(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB) const;
};

CNumberIndex::CNumberIndex(
	CFullTextIndex&		inFullTextIndex,
	CLexicon&			inLexicon,
	const string&		inName,
	uint16				inIndexNr,
	const HUrl&			inScratch)
	: CIndexBase(inFullTextIndex, inLexicon, inName, inIndexNr, inScratch, kNumberIndex)
{
}

int CNumberIndex::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	CIndexTraits<kNumberIndex> comp;
	return comp.Compare(inA, inLengthA, inB, inLengthB);
}

// 

CIndexer::CIndexer(
	const HUrl&			inDb,
	CLexicon&			inLexicon)
	: fDb(inDb.GetURL())
	, fFullTextIndex(nil)
	, fNextIndexID(0)
	, fLexicon(&inLexicon)
	, fFile(nil)
	, fHeader(new SIndexHeader)
	, fParts(nil)
	, fDocWeights(nil)
	, fOffset(0)
	, fSize(0)
{
	fHeader->sig = kIndexSig;
	fHeader->count = 0;
	fHeader->entries = 0;
	fHeader->weight_bit_count = WEIGHT_BIT_COUNT;
	fHeader->array_compression_kind = kAC_SelectorCodeV2;

	HUrl url(fDb + ".fulltext_indx");
	fFullTextIndex = new CFullTextIndex(url, fHeader->weight_bit_count);
}

CIndexer::CIndexer(HStreamBase& inFile, int64 inOffset, int64 inSize)
	: fFullTextIndex(nil)
	, fNextIndexID(0)
	, fLexicon(nil)
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
	
	for (uint32 i = 0; i < fHeader->count; ++i)	// backward compatibility, fill in tree_size and bits_size
	{
		if (fParts[i].tree_size != 0)	// not needed?
			break;

		int64 nextOffset = fOffset + fSize;
		
		if (i < fHeader->count - 1)
		{
			if (fParts[i + 1].bits_offset == 0)
				nextOffset = fParts[i + 1].tree_offset;
			else
				nextOffset = min(fParts[i + 1].tree_offset, fParts[i + 1].bits_offset);
		}
		
		if (fParts[i].bits_offset > fParts[i].tree_offset)
		{
			fParts[i].tree_size = fParts[i].bits_offset - fParts[i].tree_offset;
			fParts[i].bits_size = nextOffset - fParts[i].bits_offset;
		}
		else
		{
			if (fParts[i].bits_offset > 0)
				fParts[i].bits_size = fParts[i].tree_offset - fParts[i].bits_offset;
			fParts[i].tree_size = nextOffset - fParts[i].tree_offset;
		}
	}

	fDocWeights = new CDocWeightArray*[fHeader->count];
	memset(fDocWeights, 0, sizeof(CDocWeightArray*) * fHeader->count);
}

CIndexer::~CIndexer()
{
	if (fDocWeights != nil)
	{
		for (uint32 i = 0; i < fHeader->count; ++i)
			delete fDocWeights[i];
		delete[] fDocWeights;
	}

	delete fHeader;
	delete[] fParts;
	
	delete fFullTextIndex;
}

template<class INDEX_KIND>
inline
CIndexBase* CIndexer::GetIndexBase(const string& inIndex)
{
	CIndexBase* index = fIndices[inIndex];

	if (index == NULL)
	{
		HUrl url(fDb + '.' + inIndex + "_indx");
		index = fIndices[inIndex] =
			new INDEX_KIND(*fFullTextIndex, *fLexicon, inIndex, fNextIndexID++, url);
	}
	else if (dynamic_cast<INDEX_KIND*>(index) == NULL)
		THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));

	return index;
}

//void CIndexer::IndexText(const string& inIndex, const string& inText, bool inIndexNrs, bool inStoreIDL)
//{
//	if (inText.length() == 0)
//		return;
//	
//	CIndexBase* index = GetIndexBase<CTextIndex>(inIndex);
//	if (index->Empty())
//	{
//		if (inStoreIDL)
//			index->SetUsesInDocLocation(inStoreIDL);
//	}
//	else if (index->UsesInDocLocation() != inStoreIDL)
//		THROW(("Inconsistent use of the store IDL flag for index %s", inIndex.c_str()));
//
//	CTokenizer tok(inText.c_str(), inText.length());
//	bool isWord, isNumber, isPunct;
//	
//	while (tok.GetToken(isWord, isNumber, isPunct))
//	{
//		uint32 l = tok.GetTokenLength();
//		
//		if (isPunct or (isNumber and not inIndexNrs))
//		{
//			fFullTextIndex->Stop();
//			continue;
//		}
//		
//		if (not (isWord or isNumber) or l == 0)
//			continue;
//
//		if (l <= kMaxKeySize)
//			index->AddWord(string(tok.GetTokenValue(), l));
//		else
//			fFullTextIndex->Stop();
//	}
//}
//
//void CIndexer::IndexText(const string& inIndex, const string& inText, bool inStoreIDL)
//{
//	IndexText(inIndex, inText, false, inStoreIDL);
//}
//
//void CIndexer::IndexTextAndNumbers(const string& inIndex, const string& inText, bool inStoreIDL)
//{
//	IndexText(inIndex, inText, true, inStoreIDL);
//}

void CIndexer::IndexTokens(
	const string&			inIndexName,
	uint32					inIndexKind,
	const vector<uint32>&	inTokens)
{
	if (inTokens.size() == 0)
		return;
	
	bool inStoreIDL = true;
	CIndexBase* index = nil;

	switch (inIndexKind)
	{
		case kTextIndex:
			index = GetIndexBase<CTextIndex>(inIndexName);
			if (index->Empty())
			{
				if (inStoreIDL)
					index->SetUsesInDocLocation(inStoreIDL);
			}
			break;
		
		case kNumberIndex:
			index = GetIndexBase<CNumberIndex>(inIndexName);
			break;
		
		case kValueIndex:
			index = GetIndexBase<CValueIndex>(inIndexName);
			break;
		
		case kDateIndex:
			index = GetIndexBase<CDateIndex>(inIndexName);
			break;
		
		default:
			THROW(("Runtime error, unsupport index kind"));
	}

	if (index == nil)
		THROW(("Runtime error"));

	for (vector<uint32>::const_iterator t = inTokens.begin(); t != inTokens.end(); ++t)
	{
		if (*t)
			index->AddWord(*t);
		else
			fFullTextIndex->Stop();
	}
}

//void CIndexer::IndexWord(const string& inIndex, const string& inText)
//{
//	if (inText.length() > 0)
//	{
//		CIndexBase* index = GetIndexBase<CTextIndex>(inIndex);
//
//		if (inText.length() > kMaxKeySize)
//			THROW(("Data error: length of key too long (%s)", inText.c_str()));
//	
//		index->AddWord(tolower(inText));
//	}
//}
//
//void CIndexer::IndexDate(const string& inIndex, const string& inText)
//{
//	// inText is a formatted date which must conform to:
//	// YYYY-MM-DD
//	// Make sure it is a valid date first...
//	
//	uint32 year, month, day;
//	char *p;
//
//	if (inText.length() != 10)
//		THROW(("Invalid formatted date specified(1): '%s'", inText.c_str()));
//	
//	year = strtoul(inText.c_str(), &p, 10);
//	if (p == nil or *p != '-')
//		THROW(("Invalid formatted date specified(2): '%s'", inText.c_str()));
//	month = strtoul(p + 1, &p, 10);
//	if (p == nil or *p != '-' or month < 1 or month > 12)
//		THROW(("Invalid formatted date specified(3): '%s'", inText.c_str()));
//	day = strtoul(p + 1, &p, 10);
//	if (not (p == nil or *p == 0) or day < 1 or day > 31)
//		THROW(("Invalid formatted date specified(4): '%s'", inText.c_str()));
//	
//	struct tm tm = { 0 };
//	tm.tm_mday = day;
//	tm.tm_mon = month - 1;
//	tm.tm_year = year - 1900;
//#if P_VISUAL_CPP
//#pragma message("TODO: fix me!")
//	time_t t = mktime(&tm);
//
////	if (localtime_r(&t, &tm) == nil)
////		THROW(("Invalid formatted date specified(5): '%s'", inText.c_str()));
//#else
//	time_t t = timegm(&tm);
//
//	if (gmtime_r(&t, &tm) == nil)
//		THROW(("Invalid formatted date specified(5): '%s'", inText.c_str()));
//#endif
//	
//	if (VERBOSE > 0)
//	{
//		if (uint32(tm.tm_mday) != day or uint32(tm.tm_mon) != month - 1 or uint32(tm.tm_year + 1900) != year)
//			cerr << "Warning: Invalid formatted date specified(6): " << inText << endl;
//		//THROW(("Invalid formatted date specified(6): '%s'", inText.c_str()));
//	}
//
//	// OK, so the date specified seems to be valid, now store it as a text string
//
//	if (inText.length() > 0)
//	{
//		CIndexBase* index = fIndices[inIndex];
//		if (index == NULL)
//		{
//			HUrl url(fDb + '.' + inIndex + "_indx");
//			index = fIndices[inIndex] =
//				new CDateIndex(*fFullTextIndex, inIndex, fNextIndexID++, url);
//		}
//		else if (dynamic_cast<CDateIndex*>(index) == NULL)
//			THROW(("Inconsistent use of indexes for index %s", inIndex.c_str()));
//	
//		index->AddWord(inText);
//	}
//}
//
//void CIndexer::IndexNumber(const string& inIndex, const string& inText)
//{
//	// inText should be an integer i.e. only consisting of digits.
//	
//	for (string::const_iterator ch = inText.begin(); ch != inText.end(); ++ch)
//	{
//		if (not isdigit(*ch))
//			THROW(("Value passed to IndexNumber is not a valid integer: '%s'", inText.c_str()));
//	}
//
//	if (inText.length() > 0)
//	{
//		CIndexBase* index = GetIndexBase<CNumberIndex>(inIndex);
//	
//		index->AddWord(inText);
//	}
//}
//
//void CIndexer::IndexValue(const string& inIndex, const string& inText)
//{
//	CIndexBase* index = GetIndexBase<CValueIndex>(inIndex);
//
//	if (inText.length() > kMaxKeySize)
//		THROW(("Data error: length of unique key too long (%s)", inText.c_str()));
//
//	index->AddWord(tolower(inText));
//}
//
//void CIndexer::IndexWordWithWeight(const string& inIndex, const string& inText, uint32 inFrequency)
//{
//	CIndexBase* index = GetIndexBase<CWeightedWordIndex>(inIndex);
//
//	if (inText.length() > kMaxKeySize)
//		THROW(("Data error: length of unique key too long (%s)", inText.c_str()));
//
//	index->AddWord(tolower(inText), inFrequency);
//}

void CIndexer::FlushDoc()
{
	fFullTextIndex->FlushDoc(fHeader->entries);

	++fHeader->entries;
}

void CIndexer::CreateIndex(
	HStreamBase&	inFile,
	int64&			outOffset,
	int64&			outSize,
	bool			inCreateAllTextIndex,
	bool			inCreateUpdateDatabank,
	CDBBuildProgressMixin*
					inProgress)
{
	if (fParts)
		delete[] fParts;
	
	fParts = nil;
	fFile = &inFile;

	fHeader->sig = kIndexSig;
	fHeader->count = 0;
	
	map<string,CIndexBase*>::iterator indx;
	
	for (indx = fIndices.begin(); indx != fIndices.end(); ++indx)
	{
		if (indx->second->Empty())
		{
			delete indx->second;
			indx->second = nil;
		}
		else
		{
			indx->second->SetEmpty(true);	// reset flag
			indx->second->SetIsUpdate(inCreateUpdateDatabank);
			++fHeader->count;
		}
	}

	if (inCreateAllTextIndex)
		fHeader->count += 1; // for our allIndex

	if (VERBOSE > 0)
	{
		cout << "Flushing fulltext run buffers...";
		cout.flush();
	}
	
	if (inProgress != nil)
		inProgress->SetCreateIndexProgress(0, 0);

	fFullTextIndex->ReleaseBuffer();
	
	if (VERBOSE > 0)
		cout << " done" << endl;
	
	outOffset = inFile.Seek(0, SEEK_END);
	inFile << *fHeader;
	
	fParts = new SIndexPart[fHeader->count];
	memset(fParts, 0, sizeof(SIndexPart) * fHeader->count);
	inFile.Write(fParts, kSIndexPartSize * fHeader->count);

	// optimize invariant
	uint32 indexCount = fNextIndexID;

	// create an ordered array of indices
	HAutoBuf<CIndexBase*> indexBuf(new CIndexBase*[indexCount]);
	CIndexBase** indices = indexBuf.get();
	memset(indices, 0, sizeof(CIndexBase*) * indexCount);

	for (indx = fIndices.begin(); indx != fIndices.end(); ++indx)
	{
		if (indx->second != nil)
			indices[indx->second->GetIxNr()] = indx->second;
	}
	
	CIndexBase* allIndex = nil;
	if (inCreateAllTextIndex)
	{
		HUrl url(fDb + '.' + "alltext_indx");
		allIndex = new CWeightedWordIndex(
					*fFullTextIndex, *fLexicon, kAllTextIndexName, fHeader->count - 1, url);
	}
	
	if (VERBOSE > 0)
	{
		cout << "Creating full text indexes... ";
		cout.flush();
	}
	
	uint32 iDoc, lDoc = 0, iTerm, iIx, lTerm = 0, i, tFreq = 0;
	uint8 iFreq;

	CFullTextIndex::CRunEntryIterator iter(*fFullTextIndex);

	int64 iterCount = iter.Count();
	int64 vStep = iterCount / 10;
	if (vStep == 0)
		vStep = 1;
	
	// the next loop is very *hot*, make sure it is optimized as much as possible
	if (iter.Next(iDoc, iTerm, iIx, iFreq))
	{
		lTerm = iTerm;
		lDoc = iDoc;
		tFreq = iFreq;

		do
		{
			if (inProgress != nil and iter.Count() != 0 and (iter.Count() % 50000) == 0)
				inProgress->SetCreateIndexProgress(iterCount - iter.Count(), iterCount);
			
			if (VERBOSE and iter.Count() != 0 and (iter.Count() % vStep) == 0)
			{
				cout << (iter.Count() / vStep) << ' '; cout.flush();
			}

			if (allIndex and (lDoc != iDoc or lTerm != iTerm))
			{
				allIndex->AddDocTerm(lDoc, tFreq, iter.Bits());

				lDoc = iDoc;
				tFreq = 0;
			}
			
			if (lTerm != iTerm)
			{
				for (i = 0; i < indexCount; ++i)
				{
					if (indices[i] != nil and not indices[i]->Empty())
						indices[i]->FlushTerm(lTerm, fHeader->entries);
				}
				
				if (allIndex)
					allIndex->FlushTerm(lTerm, fHeader->entries);

				lTerm = iTerm;
				tFreq = 0;
			}
			
			indices[iIx]->AddDocTerm(iDoc, iFreq, iter.Bits());

			tFreq += iFreq;
		}
		while (iter.Next(iDoc, iTerm, iIx, iFreq));
	}

	for (i = 0; i < indexCount; ++i)
	{
		if (indices[i] != nil and not indices[i]->Empty())
			indices[i]->FlushTerm(lTerm, fHeader->entries);
	}

	if (allIndex)
	{
		allIndex->AddDocTerm(lDoc, tFreq, iter.Bits());
		allIndex->FlushTerm(lTerm, fHeader->entries);
	}

	if (VERBOSE > 0)
		cout << "done" << endl;

	if (inProgress)
		inProgress->SetCreateIndexProgress(iterCount, iterCount);
	
	uint32 ix = 0;
	for (indx = fIndices.begin(); indx != fIndices.end(); ++indx)
	{
		if (indx->second != nil)
		{
			if (inProgress != nil)
				inProgress->SetWritingIndexProgress(0, 0, indx->second->GetName().c_str());
//			mCurrentlyWritingIndex = indx->second->GetName();
			if (indx->second->Write(inFile, fHeader->entries, fParts[ix]))
				++ix;
			else
				--fHeader->count;

			delete indx->second;
		}
	}

	if (allIndex != nil)
	{
		if (inProgress != nil)
			inProgress->SetWritingIndexProgress(0, 0, allIndex->GetName().c_str());
		allIndex->Write(inFile, fHeader->entries, fParts[ix++]);
		delete allIndex;
	}
	
	outSize = inFile.Seek(0, SEEK_END) - outOffset;
	
	inFile.Seek(outOffset, SEEK_SET);

	inFile << *fHeader;

	for (ix = 0; ix < fHeader->count; ++ix)
		inFile << fParts[ix];
	
	delete fFullTextIndex;
	fFullTextIndex = nil;

	fDocWeights = new CDocWeightArray*[fHeader->count];
	memset(fDocWeights, 0, sizeof(CDocWeightArray*) * fHeader->count);
}

struct CMergeData
{
						CMergeData()
							: count(0)
							, indx(nil)
							, data(nil)
							, mappedBits(nil)
						{
						}

	vector<SIndexPart>	info;
	uint32				count;
	uint32				array_compression_kind;
	
	// cached data:
	CIndex*				indx;
	HStreamBase*		data;
	HStreamBase*		mappedBits;
};

struct CCompareIndexInfo
{
	bool operator()(const SIndexPart& inA, const SIndexPart& inB) const
	{
		return strcmp(inA.name, inB.name) < 0;
	}
};

// the next class is needed because we run out of memory with huge indices.
// profiling revealed this code to be quite performance critical, so we
// use a similar storage mechanism as in the BTree.
// Again, to save space and bandwidth we optimize a bit. This limits
// the value that can be stored to 44 bits, the same maximum as used in the
// BTree structure

const uint32 kMergeIndexBufferPageSize = 1024 * 1024;	// 1 Mb pages

class CMergeIndexBuffer
{
	typedef pair<string,int64>	value_type;
	
	struct CMergeIndexBufferPage
	{
		int32			n;
		char			s[kMergeIndexBufferPageSize - sizeof(int64) - sizeof(uint32)];
		int64			e[1];
		
		bool			AddData(const string& inString, int64 inValue);
		void			GetData(int32 inIndex, string& outString, int64& outValue) const;
	};
	
  public:
						CMergeIndexBuffer(const string& inBaseUrl);
						~CMergeIndexBuffer();

	void				push_back(const value_type& inData);
	
	int64				GuessTreeSize() const			{ return fFile->Size() + 2 * fCount * sizeof(uint32); }

	struct iterator : public boost::iterator_facade<iterator,
		const value_type, boost::forward_traversal_tag>
	{
						iterator(const iterator& inOther);
		iterator&		operator=(const iterator& inOther);
		
	  private:
		friend class boost::iterator_core_access;
		friend class CMergeIndexBuffer;

						iterator(HStreamBase& inFile, uint32 inCount);
	
		void			increment();
		void			decrement() { assert(false); }
		bool			equal(const iterator& inOther) const;
		reference		dereference() const;

		HStreamBase*	fFile;
		int64			fFileOffset;
		int32			fIndex;
		CMergeIndexBufferPage
						fPage;
		value_type		fCurrent;
		uint32			fCount;
	};
	
	iterator			begin()			{ if (fBufferPage.n > 0) FlushBuffer(); return iterator(*fFile, fCount); }
	iterator			end()			{ if (fBufferPage.n > 0) FlushBuffer(); return iterator(*fFile, 0); }
	
  private:	

	void				FlushBuffer();

	auto_ptr<HStreamBase>	fFile;
	CMergeIndexBufferPage	fBufferPage;
	uint32					fCount;
};

CMergeIndexBuffer::CMergeIndexBuffer(const string& inBaseUrl)
	: fFile(new HTempFileStream(HUrl(inBaseUrl + "_merge_indx")))
	, fCount(0)
{
	assert(sizeof(CMergeIndexBufferPage) == kMergeIndexBufferPageSize);
	fBufferPage.n = 0;
	fBufferPage.e[0] = 0;
}

CMergeIndexBuffer::~CMergeIndexBuffer()
{
}

void CMergeIndexBuffer::push_back(const value_type& inValue)
{
	if (not fBufferPage.AddData(inValue.first, inValue.second))
	{
		FlushBuffer();
		bool b = fBufferPage.AddData(inValue.first, inValue.second);
		assert(b);
		if (not b)
			THROW(("error writing buffer page"));
	}
	++fCount;
}

void CMergeIndexBuffer::FlushBuffer()
{
	fFile->Seek(0, SEEK_END);
	fFile->Write(&fBufferPage, kMergeIndexBufferPageSize);
	fFile->Seek(0, SEEK_END);	// why? I don't know... just being paranoid
	fBufferPage.n = 0;
}

bool CMergeIndexBuffer::CMergeIndexBufferPage::AddData(const string& inString, int64 inValue)
{
	bool result = false;

	uint32 sLen = inString.length();
	uint32 sOffset = static_cast<uint32>(e[-n] & 0x0FFFFFFULL);
	
	int32 free = kMergeIndexBufferPageSize;
	free -= (n + 2) * sizeof(int64) + sizeof(uint32) + sOffset;
	
	if (free > static_cast<int32>(inString.length() + sizeof(int64)))
	{
		assert(sOffset + sLen <= kMergeIndexBufferPageSize);
		
		inString.copy(s + sOffset, sLen);
		
		e[-n] = sOffset | (inValue << 24);
		e[-n - 1] = sOffset + sLen;
	
		++n;
		result = true;
	}
	
	return result;
}

void CMergeIndexBuffer::CMergeIndexBufferPage::GetData(int32 inIndex, string& outString, int64& outValue) const
{
	assert(inIndex >= 0 and inIndex < n);
	
	uint32 sOffset = static_cast<uint32>(e[-inIndex] & 0x0FFFFFFULL);
	uint32 sLen = static_cast<uint32>(e[-inIndex - 1] & 0x0FFFFFFULL) - sOffset;
	outString.assign(s + sOffset, sLen);
	outValue = e[-inIndex] >> 24;
}

CMergeIndexBuffer::iterator::iterator(HStreamBase& inFile, uint32 inCount)
	: fFile(&inFile)
	, fFileOffset(0)
	, fIndex(0)
	, fCount(inCount)
{
	if (fCount > 0)
		fFile->PRead(&fPage, kMergeIndexBufferPageSize, fFileOffset);
	else
		fPage.n = 0;
	
	if (fIndex < fPage.n)
		fPage.GetData(fIndex, fCurrent.first, fCurrent.second);
}

CMergeIndexBuffer::iterator::iterator(const iterator& inOther)
	: fFile(inOther.fFile)
	, fFileOffset(inOther.fFileOffset)
	, fIndex(inOther.fIndex)
	, fPage(inOther.fPage)
	, fCurrent(inOther.fCurrent)
	, fCount(inOther.fCount)
{
}

CMergeIndexBuffer::iterator& CMergeIndexBuffer::iterator::operator=(const iterator& inOther)
{
	fFile = inOther.fFile;
	fFileOffset = inOther.fFileOffset;
	fIndex = inOther.fIndex;
	fCount = inOther.fCount;
	fPage = inOther.fPage;
	fCurrent = inOther.fCurrent;
	return *this;
}

void CMergeIndexBuffer::iterator::increment()
{
	if (--fCount > 0)
	{
		if (++fIndex >= fPage.n)
		{
			fFileOffset += kMergeIndexBufferPageSize;
			uint32 r = fFile->PRead(&fPage, kMergeIndexBufferPageSize, fFileOffset);
			if (r != kMergeIndexBufferPageSize)
				THROW(("Error reading merge index buffer page (count = %d, r = %d)", fCount, r));
			
			fIndex = 0;
		}
		
		assert(fPage.n);
		
		if (fPage.n == 0)
			THROW(("Merge index buffer page is invalid"));
		
		fPage.GetData(fIndex, fCurrent.first, fCurrent.second);
	}
}

bool CMergeIndexBuffer::iterator::equal(const iterator& inOther) const
{
	return this == &inOther or
		(fFile == inOther.fFile and fCount == inOther.fCount);
}

CMergeIndexBuffer::iterator::reference CMergeIndexBuffer::iterator::dereference() const
{
	return fCurrent;
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
		const CIndexer* index = (*p)->GetIndexer();

		fHeader->entries += index->fHeader->entries;
		
		CMergeData m;
		
		for (ix = 0; ix < index->fHeader->count; ++ix)
			m.info.push_back(index->fParts[ix]);
		m.count = index->fHeader->entries;
		m.array_compression_kind = index->fHeader->array_compression_kind;

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
			n->copy(info.name, sizeof(info.name) - 1);
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

		for (i = 0; fParts[ix].kind == 0 and i < md.size(); ++i)
			fParts[ix].kind = md[i].info[ix].kind;
		
		auto_ptr<CJoinedIteratorBase> iter(CJoinedIteratorBase::Create(fParts[ix].kind));
		
		for (i = 0; i < md.size(); ++i)
		{
			md[i].indx = nil;
			
			if (md[i].info[ix].kind == 0)	// this index is not available in this part
				continue;
			
			if (fParts[ix].kind != md[i].info[ix].kind)
				THROW(("Incompatible index types"));

			md[i].data = &inParts[i]->GetDataFile();
			md[i].indx = new CIndex(fParts[ix].kind, md[i].info[ix].index_version,
				*md[i].data, md[i].info[ix].tree_offset, md[i].info[ix].root);

			uint32 delta = 0;
			if (fParts[ix].kind == kValueIndex)
				delta = count;
			
			iter->Append(new CIteratorWrapper<CIndex>(*md[i].indx, delta), i);
			
			if (md[i].info[ix].bits_size > 0)
			{
				try	// see if we can memory map the bits section
				{
					HFileStream* df = dynamic_cast<HFileStream*>(md[i].data);
					if (df != nil)
					{
						md[i].mappedBits = new HMMappedFileStream(*df,
							md[i].info[ix].bits_offset, md[i].info[ix].bits_size);
					}
				}
				catch (...) {}
				
				if (md[i].mappedBits == nil)
				{
					md[i].mappedBits = new HStreamView(*md[i].data,
						md[i].info[ix].bits_offset, md[i].info[ix].bits_size);
				}
			}
			else
				md[i].mappedBits = nil;
			
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

		fParts[ix].root = 0;
		fParts[ix].bits_offset = 0;
		
		CMergeIndexBuffer indx(fDb);
		string s;
		
		fParts[ix].entries = 0;
		fParts[ix].sig = kIndexPartSig;
		fParts[ix].index_version = kCIndexVersionV1;
		
		vector<pair<uint32,int64> > v;
		
		while (iter->Next(s, v))
		{
			if (VERBOSE > 1)
			{
				if (fParts[ix].kind == kValueIndex and v.size() > 1)
					cerr << "Duplicate key '" << s << "' in index '" << fParts[ix].name << '\'' << endl;
			}

			++fParts[ix].entries;
			
			switch (fParts[ix].kind)
			{
				case kValueIndex:
				{
					vector<pair<uint32,int64> >::iterator m;
					
					// in case of a duplicate ID we store the ID with the highest
					// document ID. 
					if (v.size() > 1)
					{
						m = max_element(v.begin(), v.end(),
							bind(&pair<uint32,int64>::second, _1) < bind(&pair<uint32,int64>::second, _2));
					
						if (m == v.end())
							THROW(("runtime error"));
					}
					else
						m = v.begin();

					indx.push_back(make_pair(s, m->second));
					break;
				}
				
				case kTextIndex:
					if ((fParts[ix].flags & kContainsIDL) != 0)
					{
						uint32 first = 0;
						
						boost::ptr_vector<CDbDocIteratorBase>	docIters;
						uint32 count = 0;
	
						for (i = 0; i < md.size(); ++i)
						{
							for (uint32 j = 0; j < v.size(); ++j)
							{
								if (v[j].first == i)
								{
									CDbDocIteratorBase* iter =
										CreateDbDocIterator(md[i].array_compression_kind, *md[i].mappedBits,
											v[j].second, md[i].count, first, true);
									
									if (dynamic_cast<CDbIDLDocIteratorSC2*>(iter) == nil)
										THROW(("Inconsistent use of IDL containing iterators"));
									
									docIters.push_back(iter);

									count += docIters.back().Count();
									break;
								}
							}
							first += md[i].count;
						}
						
						HMemoryStream idlBuffer;
						COBitStream idlBits(idlBuffer);
						
						vector<uint32> docs;
						docs.reserve(count);

						for (i = 0; i < docIters.size(); ++i)
						{
							CDbIDLDocIteratorSC2& iter = static_cast<CDbIDLDocIteratorSC2&>(docIters[i]);
							
							uint32 doc;
	
							while (iter.Next(doc, idlBits, false))
								docs.push_back(doc);
						}
	
						int64 offset = bitFile->Seek(0, SEEK_END);
						if (offset > numeric_limits<uint32>::max())
						{
							fParts[ix].sig = kIndexPartSigV2;
							fParts[ix].index_version = kCIndexVersionV2;
						}
						
						idlBits.sync();
						CIDLInterleaver interleaver(idlBuffer);
	
						COBitStream bits(*bitFile.get());
						CompressArray(bits, docs, fHeader->entries, &interleaver);
						
						indx.push_back(make_pair(s, offset));
						break;
					} // else fall through...
					
				case kDateIndex:
				case kNumberIndex:
				{
					uint32 first = 0;
					
					boost::ptr_vector<CDbDocIteratorBase>	docIters;
					uint32 count = 0;

					for (i = 0; i < md.size(); ++i)
					{
						for (uint32 j = 0; j < v.size(); ++j)
						{
							if (v[j].first == i)
							{
								docIters.push_back(
									CreateDbDocIterator(md[i].array_compression_kind, *md[i].mappedBits,
										v[j].second, md[i].count, first, false));

								count += docIters.back().Count();
								break;
							}
						}
						first += md[i].count;
					}
					
					vector<uint32> docs;
					docs.reserve(count);

					for (i = 0; i < docIters.size(); ++i)
					{
						CDbDocIteratorBase& iter = docIters[i];
						
						uint32 doc;

						while (iter.Next(doc, false))
							docs.push_back(doc);
					}

					int64 offset = bitFile->Seek(0, SEEK_END);
					if (offset > numeric_limits<uint32>::max())
					{
						fParts[ix].sig = kIndexPartSigV2;
						fParts[ix].index_version = kCIndexVersionV2;
					}

					COBitStream bits(*bitFile.get());
					CompressArray(bits, docs, fHeader->entries);
					
					indx.push_back(make_pair(s, offset));
					break;
				}
				
				case kWeightedIndex:
				{
					uint32 first = 0;
					
					boost::ptr_vector<CDbDocIteratorBase>	docIters;
					uint32 count = 0;

					for (i = 0; i < md.size(); ++i)
					{
						for (uint32 j = 0; j < v.size(); ++j)
						{
							if (v[j].first == i)
							{
								docIters.push_back(CreateDbDocWeightIterator(md[i].array_compression_kind,
									*md[i].mappedBits, v[j].second, md[i].count, first));

								count += docIters.back().Count();
								break;
							}
						}
						first += md[i].count;
					}
					
					float idf = log(1.0 + static_cast<double>(fHeader->entries) / count);

					vector<pair<uint32,uint8> > docs;
					docs.reserve(count);

					for (i = 0; i < docIters.size(); ++i)
					{
						CDbDocIteratorBase& iter = docIters[i];
						
						uint32 doc;
						uint8 freq;

						while (iter.Next(doc, freq, false))
						{
							docs.push_back(make_pair(doc, freq));

							assert(doc < fHeader->entries);

							float wdt = freq * idf;
							dw[doc] += wdt * wdt;
						}
					}
					
					int64 offset = bitFile->Seek(0, SEEK_END);
					if (offset > numeric_limits<uint32>::max())
					{
						fParts[ix].sig = kIndexPartSigV2;
						fParts[ix].index_version = kCIndexVersionV2;
					}

					COBitStream bits(*bitFile.get());
					CompressArray(bits, docs, fHeader->entries);
					
					indx.push_back(make_pair(s, offset));
					break;
				}
			}
		}
		
		CIteratorWrapper<CMergeIndexBuffer> mapIter(indx);

		if (indx.GuessTreeSize() > numeric_limits<int32>::max())
		{
			fParts[ix].sig = kIndexPartSigV2;
			fParts[ix].index_version = kCIndexVersionV2;
		}

		auto_ptr<CIndex> indxOnDisk(CIndex::CreateFromIterator(fParts[ix].kind,
			fParts[ix].index_version, mapIter, outData));
		
		fParts[ix].tree_offset = indxOnDisk->GetOffset();
		fParts[ix].tree_size = outData.Seek(0, SEEK_END) - fParts[ix].tree_offset;
		fParts[ix].root = indxOnDisk->GetRoot();
		
		if (fParts[ix].kind != kValueIndex)
		{
			fParts[ix].bits_offset = outData.Seek(0, SEEK_END);
			fParts[ix].bits_size = bitFile->Size();
			
			bitFile->CopyTo(outData, bitFile->Size(), 0);
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
			
			delete md[i].mappedBits;
			md[i].mappedBits = nil;
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
	const string& inValue) const
{
	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(inIndex);
	
	auto_ptr<CDocIterator> result(nil);
	
	uint32 ix = 0;
	while (ix < fHeader->count and index != fParts[ix].name)
		++ix;

	if (ix == fHeader->count)
		THROW(("Index '%s' not found", inIndex.c_str()));

	auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
		*fFile, fParts[ix].tree_offset, fParts[ix].root));

	auto_ptr<vector<uint32> > values(new vector<uint32>());
	indx->GetValuesForPattern(inValue, *values.get());
	
	if (values->size())
	{
		if (fParts[ix].kind == kValueIndex)
			result.reset(new CDocVectorIterator(values.release()));
		else if (fParts[ix].kind == kTextIndex or fParts[ix].kind == kDateIndex or fParts[ix].kind == kNumberIndex)
		{
			vector<CDocIterator*> ixs;

			for (vector<uint32>::iterator i = values->begin(); i != values->end(); ++i)
				ixs.push_back(CreateDbDocIterator(fHeader->array_compression_kind, *fFile,
					fParts[ix].bits_offset + *i, fHeader->entries, 0, (fParts[ix].flags & kContainsIDL) != 0));

			result.reset(CDocUnionIterator::Create(ixs));
		}
	}

	return result.release();
}

CDocIterator* CIndexer::CreateDocIterator(const string& inIndex, const string& inKey,
	bool inKeyIsPattern, CQueryOperator inOperator) const
{
	vector<CDocIterator*> iters;

	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(inIndex);

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
			
			auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
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
				else if (values.size() > 100)
				{
					auto_ptr<CDocBitVectorIterator> bitIter(new CDocBitVectorIterator(fHeader->entries));
					
					for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
					{
						if (fParts[ix].kind == kWeightedIndex)
							bitIter->Add(CreateDbDocWeightIterator(fHeader->array_compression_kind, *fFile,
								fParts[ix].bits_offset + *i, fHeader->entries));
						else
							bitIter->Add(CreateDbDocIterator(fHeader->array_compression_kind, *fFile,
								fParts[ix].bits_offset + *i, fHeader->entries, 0, (fParts[ix].flags & kContainsIDL) != 0));
					}
					
					iters.push_back(bitIter.release());
				}				
				else
				{
					for (vector<uint32>::iterator i = values.begin(); i != values.end(); ++i)
					{
						if (fParts[ix].kind == kWeightedIndex)
							iters.push_back(CreateDbDocWeightIterator(fHeader->array_compression_kind, *fFile,
								fParts[ix].bits_offset + *i, fHeader->entries));
						else
							iters.push_back(CreateDbDocIterator(fHeader->array_compression_kind, *fFile,
								fParts[ix].bits_offset + *i, fHeader->entries, 0, (fParts[ix].flags & kContainsIDL) != 0));
					}
				}
			}
		}
		else
		{
			auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
				*fFile, fParts[ix].tree_offset, fParts[ix].root));
		
			int64 value;
			if (indx->GetValue(key, value))
			{
				if (fParts[ix].kind == kValueIndex)
					iters.push_back(new CDocNrIterator(value));
				else if (fParts[ix].kind == kWeightedIndex)
					iters.push_back(CreateDbDocWeightIterator(fHeader->array_compression_kind, *fFile,
						fParts[ix].bits_offset + value, fHeader->entries));
				else
					iters.push_back(CreateDbDocIterator(fHeader->array_compression_kind, *fFile,
						fParts[ix].bits_offset + value, fHeader->entries, 0, (fParts[ix].flags & kContainsIDL) != 0));
			}
		}
	}
	
	return CDocUnionIterator::Create(iters);
}

CDocIterator* CIndexer::CreateDocIteratorForPhrase(const string& inIndex, const vector<string>& inPhrase) const
{
	vector<CDocIterator*> iters;

	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(inIndex);

	for (uint32 ix = 0; ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name and index != "*")
			continue;
		
		if ((fParts[ix].flags & kContainsIDL) == 0 or fParts[ix].kind != kTextIndex)
			continue;

		auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
			*fFile, fParts[ix].tree_offset, fParts[ix].root));
	
		vector<int64> values;
		
		for (vector<string>::const_iterator term = inPhrase.begin(); term != inPhrase.end(); ++term)
		{
			int64 value;

			if (indx->GetValue(*term, value))
				values.push_back(value);
			else
				break;
		}
		
		if (values.size() == inPhrase.size())
		{
			vector<CDocIterator*> termIters;
			
			for (vector<int64>::iterator value = values.begin(); value != values.end(); ++value)
			{
				termIters.push_back(CreateDbDocIterator(fHeader->array_compression_kind, *fFile,
					fParts[ix].bits_offset + *value, fHeader->entries, 0, true));
			}
			
			if (fHeader->array_compression_kind == kAC_SelectorCodeV2)
				iters.push_back(new CDbPhraseDocIterator<kAC_SelectorCodeV2>(termIters));
			else if (fHeader->array_compression_kind == kAC_SelectorCodeV1)
				iters.push_back(new CDbPhraseDocIterator<kAC_SelectorCodeV1>(termIters));
			else
				iters.push_back(new CDbPhraseDocIterator<kAC_GolombCode>(termIters));
		}
	}
	
	return CDocUnionIterator::Create(iters);
}

uint32 CIndexer::Count() const
{
	return fHeader->entries;
}

void CIndexer::PrintInfo() const
{
	const char* sig = reinterpret_cast<const char*>(&fHeader->sig);
	
	cout << "Index Header ("
		 << sig[0] << sig[1] << sig[2] << sig[3] << ") " << fHeader->size << " bytes" << endl;
	cout << "  entries:           " << fHeader->entries << endl;
	cout << "  count:             " << fHeader->count << endl;
	cout << "  weight bit count:  " << fHeader->weight_bit_count << endl;

	char ck[5] = { 0 };
	snprintf(ck, sizeof(ck), "%4.4s", reinterpret_cast<const char*>(&fHeader->array_compression_kind));
	
	cout << "  array compression: " << ck << endl;
	cout << endl;
	
	for (uint32 ix = 0; ix < fHeader->count; ++ix)
	{
		SIndexPart& p = fParts[ix];
		
		sig = reinterpret_cast<const char*>(&p.sig);
		
		cout << "Index Part " << ix << " ("
			 << sig[0] << sig[1] << sig[2] << sig[3] << ") " << fHeader->size << " bytes" << endl;

		cout << "  name:          " << p.name << endl;
		
		sig = reinterpret_cast<const char*>(&p.kind);
		cout << "  kind:          " << sig[0] << sig[1] << sig[2] << sig[3] << endl;
		cout << "  tree offset:   " << p.tree_offset << endl;
		cout << "  tree size:     " << p.tree_size << endl;
		cout << "  root:          " << p.root << endl;
		cout << "  entries:       " << p.entries << endl;
		if (p.kind != kValueIndex)
		{
			cout << "  bits offset:   " << p.bits_offset << endl;
			cout << "  bits size:     " << p.bits_size << endl;
		}
		if (p.kind == kWeightedIndex)
			cout << "  weight offset: " << p.weight_offset << endl;

		if (p.flags)
		{
			cout << "  flags:         ";
			if (p.flags & kContainsIDL)
				cout << "IDL ";
			cout << endl;
		}

		cout << endl;
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

CIteratorBase* CIndexer::GetIteratorForIndex(const string& inIndex) const
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

CIteratorBase* CIndexer::GetIteratorForIndexAndKey(const string& inIndex, const string& inKey) const
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

CDocWeightArray CIndexer::GetDocWeights(const std::string& inIndex) const
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
	
	if (fDocWeights == nil)
		THROW(("fDocWeights is nil"));
	
	if (fDocWeights[ix] == nil)
	{
		// lock and try again...
		static HMutex mutex;
		StMutex lock(mutex);
		
		if (fDocWeights[ix] == nil)
		{
			HFileStream* file = dynamic_cast<HFileStream*>(fFile);
			if (file == nil)
				THROW(("Stream should be a file in GetDocWeights"));
		
			fDocWeights[ix] = new CDocWeightArray(*file, fParts[ix].weight_offset, fHeader->entries);
		}
	}
	
	return *fDocWeights[ix];
}

CDbDocIteratorBase* CIndexer::GetDocWeightIterator(const string& inIndex, const string& inKey) const
{
	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(index);

	CDbDocIteratorBase* result = nil;
	
	for (uint32 ix = 0; result == nil and ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name)
			continue;

		auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
			*fFile, fParts[ix].tree_offset, fParts[ix].root));

		int64 value;
		if (indx->GetValue(inKey, value))
			result = CreateDbDocWeightIterator(fHeader->array_compression_kind,
				*fFile, fParts[ix].bits_offset + value, fHeader->entries);

		break;
	}

	return result;
}

CIndex* CIndexer::GetIndex(const string& inIndex) const
{
	string index = inIndex;
	if (index != kAllTextIndexName)
		index = tolower(index);
	
	CIndex* result = nil;

	for (uint32 ix = 0; result == nil and ix < fHeader->count; ++ix)
	{
		if (index != fParts[ix].name)
			continue;

		result = new CIndex(fParts[ix].kind, fParts[ix].index_version,
			*fFile, fParts[ix].tree_offset, fParts[ix].root);
	}
	
	return result;
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

bool CIndexer::GetDocumentNr(const string& inDocumentID, uint32& outDocNr) const
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
					CWeightAccumulator(uint32 inKind, HStreamBase& inFile,
							float inWeights[], uint32 inDocCount)
						: fKind(inKind)
						, fFile(inFile)
						, fWeights(inWeights)
						, fDocCount(inDocCount)
					{
					}
	
	void			Visit(const string& inKey, int64 inValue);

  private:
	uint32			fKind;
	HStreamBase&	fFile;
	float*			fWeights;
	uint32			fDocCount;
};

void CWeightAccumulator::Visit(const string& inKey, int64 inValue)
{
	auto_ptr<CDbDocIteratorBase> iter(CreateDbDocWeightIterator(fKind, fFile, inValue, fDocCount));
	
	float idf = iter->GetIDFCorrectionFactor();
	
	uint32 doc;
	uint8 freq;

	while (iter->Next(doc, freq, false))
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

		indx.reset(new CIndex(fParts[ix].kind, fParts[ix].index_version,
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
	CWeightAccumulator accu(fHeader->array_compression_kind, bits, dw, docCount);
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

struct CIndexTester
{
					CIndexTester(
						uint32			inKind,
						HStreamBase&	inFile,
						uint32			inDocCount,
						uint32			inIxCount,
						uint32			inFlags,
						bool			inExitOnFailure)
						: fKind(inKind)
						, fFile(inFile)
						, fDocCount(inDocCount)
						, fCount(0)
						, fIxCount(inIxCount)
						, fTick(inIxCount / 60)
						, fFlags(inFlags)
						, fLastValue(0)
						, fExitOnFailure(inExitOnFailure)
					{
					}
	
	void			VisitValue(const string& inKey, int64 inValue);
	void			VisitSimple(const string& inKey, int64 inValue);
	void			VisitWeighted(const string& inKey, int64 inValue);

	uint32			fKind;
	HStreamBase&	fFile;
	uint32			fDocCount;
	uint32			fCount;
	uint32			fIxCount;
	uint32			fTick;
	uint32			fFlags;
	int64			fLastValue;
	bool			fExitOnFailure;
};

void CIndexTester::VisitValue(const string& inKey, int64 inValue)
{
	if (inValue >= fDocCount)
	{
		cerr << "key: " << inKey << " value: " << inValue << " <= out of range" << endl;
		if (fExitOnFailure) exit(1);
	}
	
	if (++fCount > fDocCount)
	{
		cerr << "Too many index entries" << endl;
		if (fExitOnFailure) exit(1);
	}
	
	if (fTick > 0 and (fCount % fTick) == 0)
	{
		cout << ".";
		cout.flush();
	}
}

void CIndexTester::VisitSimple(const string& inKey, int64 inValue)
{
	++fCount;

	if (fTick > 0 and (fCount % fTick) == 0)
	{
		cout << ".";
		cout.flush();
	}

	if (fCount > fIxCount)
	{
		cerr << "Too many entries in index" << endl;
		fCount = 0;
		if (fExitOnFailure) exit(1);
	}

	if (inValue <= fLastValue and inValue > 0)
	{
		cerr << "Value is not increasing "
			 << " key = " << inKey
			 << " previous value = " << fLastValue
			 << " value = " << inValue << endl;
		if (fExitOnFailure) exit(1);
	}

	fLastValue = inValue;

	try
	{
		auto_ptr<CDbDocIteratorBase> iter(
			CreateDbDocIterator(fKind, fFile, inValue, fDocCount, 0, (fFlags & kContainsIDL) != 0));
		
		uint32 doc;
		uint32 cnt = iter->Count(), n = 0;
	
		while (iter->Next(doc, false))
		{
			if (doc >= fDocCount)
			{
				cerr << "key: " << inKey << " value: " << inValue << " doc: " << doc << " <= out of range" << endl;
				if (fExitOnFailure) exit(1);
				break;
			}
			
			if (++n > cnt)
			{
				cerr << "key: " << inKey << " value: " << inValue << " n: " << n << " too many entries in posting list, cnt: " << cnt << endl;
				if (fExitOnFailure) exit(1);
				break;
			}
		}
	}
	catch (HError& e)
	{
		cerr << "Exception caught: " << e.what() << endl
			 << "key value was: " << inKey << endl;
		if (fExitOnFailure) exit(1);
	}
	catch (...)
	{
		cerr << "Unknown exception caught" << endl
			 << "key value was: " << inKey << endl;
		if (fExitOnFailure) exit(1);
	}
}

void CIndexTester::VisitWeighted(const string& inKey, int64 inValue)
{
	++fCount;

	if (fTick > 0 and (fCount % fTick) == 0)
	{
		cout << ".";
		cout.flush();
	}

	if (fCount > fIxCount)
	{
		cerr << "Too many entries in index" << endl;
		fCount = 0;
		if (fExitOnFailure) exit(1);
	}

	if (inValue <= fLastValue and inValue > 0)
	{
		cerr << "Value is not increasing "
			 << " key = " << inKey
			 << " previous value = " << fLastValue
			 << " value = " << inValue << endl;
		if (fExitOnFailure) exit(1);
	}

	fLastValue = inValue;

	auto_ptr<CDbDocIteratorBase> iter(CreateDbDocWeightIterator(fKind, fFile, inValue, fDocCount));
	
	uint32 doc;
	uint8 freq;
	uint32 cnt = iter->Count(), n = 0;

	try
	{
		while (iter->Next(doc, freq, false))
		{
			if (doc >= fDocCount)
			{
				cerr << "key: " << inKey << " value: " << inValue << " doc: " << doc << " <= out of range" << endl;
				if (fExitOnFailure) exit(1);
				break;
			}
	
			if (freq == 0)
			{
				cerr << "key: " << inKey << " value: " << inValue << " zero frequency" << endl;
				if (fExitOnFailure) exit(1);
				break;
			}
	
			if (++n > cnt)
			{
				cerr << "key: " << inKey << " value: " << inValue << " n: " << n << " too many entries in posting list, cnt: " << cnt << endl;
				if (fExitOnFailure) exit(1);
				break;
			}
		}
	}
	catch (HError& e)
	{
		cerr << "Exception caught: " << e.what() << endl
			 << "key value was: " << inKey << endl;
		if (fExitOnFailure) exit(1);
	}
	catch (...)
	{
		cerr << "Unknown exception caught" << endl
			 << "key value was: " << inKey << endl;
		if (fExitOnFailure) exit(1);
	}
}

void CIndexer::Test(
	bool		inExitOnFailure) const
{
	uint32 ix;
	for (ix = 0; ix < fHeader->count; ++ix)
	{
		cout << "Testing index " << fParts[ix].name << endl;
		
		auto_ptr<CIndex> indx(new CIndex(fParts[ix].kind, fParts[ix].index_version,
			*fFile, fParts[ix].tree_offset, fParts[ix].root));

		int64 bitsOffset = fParts[ix].bits_offset;
		HStreamView bits(*fFile, fParts[ix].bits_offset, fFile->Size() - bitsOffset);

		CIndexTester tester(
			fHeader->array_compression_kind, bits,
			fHeader->entries, fParts[ix].entries, fParts[ix].flags,
			inExitOnFailure);

		switch (fParts[ix].kind)
		{
			case kValueIndex:
				indx->Visit(&tester, &CIndexTester::VisitValue);
				break;

			case kWeightedIndex:
				indx->Visit(&tester, &CIndexTester::VisitWeighted);
				break;

			default:
				indx->Visit(&tester, &CIndexTester::VisitSimple);
				break;
		}
		
		cout << " done" << endl;
	}
}
