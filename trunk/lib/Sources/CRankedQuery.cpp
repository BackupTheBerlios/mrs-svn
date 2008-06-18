/*	$Id: CRankedQuery.cpp 324 2007-02-08 12:36:05Z hekkel $
	Copyright hekkelman
	Created Monday February 20 2006 14:06:39
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

#include <vector>
#include "HUtils.h"
#include <iostream>
#include "HError.h"

#include <boost/ptr_container/ptr_vector.hpp>

#include "CRankedQuery.h"
#include "CDatabank.h"
#include "CIndexer.h"
#include "CDocWeightArray.h"
#include "CTokenizer.h"

#include "CUtils.h"

using namespace std;

struct Term
{
	string							key;
	uint32							weight;
	auto_ptr<CDbDocIteratorBase>	iter;
	float							w;

									Term()
										: weight(0)
										, w(0) {}
									
									Term(const Term& inOther)
										: key(inOther.key)
										, weight(inOther.weight)
										, iter(nil)
										, w(inOther.w) {}

	Term&							operator=(const Term& inOther)
									{
										key = inOther.key;
										weight = inOther.weight;
										iter.release();
										w = inOther.w;
										return *this;
									}
	
	bool operator<(const Term& inOther) const
			{ return w < inOther.w; }

	bool operator>(const Term& inOther) const
			{ return w > inOther.w; }
};

typedef boost::ptr_vector<Term>	TermVector;

struct CRankedQueryImp
{
	TermVector		fTerms;
};

struct CDocScore
{
	uint32			fDocNr;
	float			fRank;
	
	bool			operator<(const CDocScore& inOther) const
						{ return fRank < inOther.fRank; }
	bool			operator>(const CDocScore& inOther) const
						{ return fRank > inOther.fRank; }
};

class CAccumulator
{
	friend class Iterator;
	friend struct ItemRef;

	struct Item
	{
		float			value;
		uint32			count;
		Item*			next;
	};

  public:

	struct ItemRef
	{
		friend class CAccumulator;
		
		float		operator+=(float inValue)
					{
						if (fItem->count == 0)
						{
							fItem->next = fA.fFirst;
							fA.fFirst = fItem;
							++fA.fHitCount;
						}
						
						fItem->value += inValue;
						fItem->count += 1;

						return fItem->value;
					}

					operator float() const
					{
						return fItem->value;
					}
		
		uint32		Count() const;

	  private:

					ItemRef(CAccumulator& inAccumulator, Item inData[], uint32 inDocNr)
						: fItem(inData + inDocNr)
						, fA(inAccumulator) {}
					ItemRef(const ItemRef&);
					ItemRef();
		ItemRef&	operator=(const ItemRef&);

		Item*			fItem;
		CAccumulator&	fA;
	};
	
	// HA!
	//
	// This code proved to be quite performance sensitive. I did a 'new Item[]'
	// first and Item had a constructor that set the data to 0. Took 1.5 seconds
	// on a fast Xeon for genbank_release. Too slow. Then I removed the constructor
	// and added a call to memset, made no difference.
	// Then I remembered that physical memory is only allocated when needed, and so
	// I decided to try calloc. That will allocate a memory page and zero it when needed,
	// often we don't need all of it. The result is that the construction of the CAccumulator
	// object now takes less than a milli second. Duh...
	
					CAccumulator(uint32 inDocumentCount)
						: fData(static_cast<Item*>(calloc(sizeof(Item), inDocumentCount)))
						, fFirst(nil)
						, fDocCount(inDocumentCount)
						, fHitCount(0)
					{
					}

	virtual			~CAccumulator()
					{
						free(fData);
					}
	
	ItemRef			operator[](uint32 inDocNr)
					{
						assert(inDocNr < fDocCount);
						return ItemRef(*this, fData, inDocNr);
					}
	
	class Iterator : public CDocIterator
	{
	  public:
						Iterator(CAccumulator& inAccumulator, uint32 inTermCount,
							bool inSortDocs);
						~Iterator();
		
		virtual bool	Next(uint32& ioDoc, bool inSkip);
		virtual uint32	Count() const						{ return fHitCount; }
		virtual uint32	Read() const						{ return fCurrent; }
		const Item&		GetItem() const						{ return fData[fDocs[fCurrent]]; }
	
	  private:
		uint32*			fDocs;
		Item*			fData;
		uint32			fDocCount;
		uint32			fHitCount;
		uint32			fCurrent;
		uint32			fTermCount;
	};
	
  private:
	Item*				fData;
	Item*				fFirst;
	uint32				fDocCount;
	uint32				fHitCount;
};

CAccumulator::Iterator::Iterator(CAccumulator& inAccumulator, uint32 inTermCount, bool inSortDocs)
	: fDocs(new uint32[inAccumulator.fHitCount])
	, fData(inAccumulator.fData)
	, fDocCount(inAccumulator.fDocCount)
	, fHitCount(inAccumulator.fHitCount)
	, fCurrent(0)
	, fTermCount(inTermCount)
{
	Item* item = inAccumulator.fFirst;
	
	if (inTermCount != 0)
	{
		uint32 i = 0;

		while (item != nil)
		{
			if (item->count == inTermCount)
			{
				fDocs[i] = item - fData;
				++i;
			}
			
			item = item->next;
		}
		
		fHitCount = i;
	}
	else
	{
		for (uint32 i = 0; i < fHitCount; ++i)
		{
			assert(item != nil);
	
			fDocs[i] = item - fData;
	
			item = item->next;
		}
		
		assert(item == nil);
	}
	
	if (inSortDocs)
		sort(fDocs, fDocs + fHitCount);
}

CAccumulator::Iterator::~Iterator()
{
	delete[] fDocs;
}

bool CAccumulator::Iterator::Next(uint32& ioDoc, bool inSkip)
{
	bool result = false;

	while (fCurrent < fHitCount)
	{
		uint32 doc = fDocs[fCurrent];
		++fCurrent;
		
		if (inSkip and doc < ioDoc)
			continue;
		
		result = true;
		ioDoc = doc;

		break;
	}
	
	return result;
}

CRankedQuery::CRankedQuery()
	: fImpl(new CRankedQueryImp)
{
}

CRankedQuery::~CRankedQuery()
{
	delete fImpl;
}

void CRankedQuery::AddTerm(const string& inKey, uint32 inFrequency)
{
	auto_ptr<Term> t(new Term);

	t->key = tolower(inKey);
	t->weight = inFrequency;
	
	fImpl->fTerms.push_back(t.release());
}

void CRankedQuery::AddTermsFromText(const string& inText)
{
	if (inText.length() == 0)
		return;
	
	CTokenizer tok(inText.c_str(), inText.length());
	bool isWord, isNumber, isPunct;
	
	map<string,uint32> words;
	
	while (tok.GetToken(isWord, isNumber, isPunct))
	{
		uint32 l = tok.GetTokenLength();

		if (not (isWord or isNumber) or l == 0)
			continue;
		
		words[string(tok.GetTokenValue(), l)] += 1;
	}
	
	for (map<string,uint32>::iterator w = words.begin(); w != words.end(); ++w)
		AddTerm(w->first, w->second);
}

struct Algorithm
{
	virtual				~Algorithm() {}

	virtual float		operator()(float inAccumulator, float inDocWeight, float inQueryWeight) = 0;
	
	static Algorithm&	Choose(const string& inName);
};

template<class T>
struct AlgorithmT : public Algorithm
{
	static T		sInstance;
};

template<class T>
T AlgorithmT<T>::sInstance;

struct VectorAlgorithm : public AlgorithmT<VectorAlgorithm>
{
	virtual float	operator()(float inAccumulator, float inDocWeight, float inQueryWeight)
						{ return inAccumulator / (inDocWeight * inQueryWeight); }
};

struct DiceAlgorithm : public AlgorithmT<DiceAlgorithm>
{
	virtual float	operator()(float inAccumulator, float inDocWeight, float inQueryWeight)
						{ return 2 * inAccumulator / (inDocWeight * inDocWeight + inQueryWeight * inQueryWeight); }
};

struct JaccardAlgorithm : public AlgorithmT<JaccardAlgorithm>
{
	virtual float	operator()(float inAccumulator, float inDocWeight, float inQueryWeight)
						{ return inAccumulator / (inDocWeight * inDocWeight + inQueryWeight * inQueryWeight - inAccumulator); }
};

Algorithm& Algorithm::Choose(const string& inName)
{
	if (inName == "vector")
		return VectorAlgorithm::sInstance;
	else if (inName == "dice")
		return DiceAlgorithm::sInstance;
	else if (inName == "jaccard")
		return JaccardAlgorithm::sInstance;
	else
		THROW(("Unknown algorithm: %s", inName.c_str()));
}

void CRankedQuery::PerformSearch(
	CDatabankBase&		inDatabank,
	const string&		inIndex,
	const string&		inAlgorithm,
	auto_ptr<CDocIterator>
						inMetaQuery,
	uint32				inMaxReturn,
	bool				inAllTermsRequired,
	auto_ptr<CDocIterator>&
						outResults,
	uint32&				outCount)
{
	Algorithm& alg = Algorithm::Choose(inAlgorithm);
	
	CDocWeightArray Wd = inDatabank.GetDocWeights(inIndex);
	const uint32 maxWeight = inDatabank.GetMaxWeight();

	set<string> stopWords;
	inDatabank.GetStopWords(stopWords);
	
	float Wq = 0, Smax = 0;
	uint32 termCount = 0;

	// normalize the term frequencies.
	
//	TermVector expandedTerms;
//	
	uint32 maxTermFreq = 1;
	for (TermVector::iterator t = fImpl->fTerms.begin(); t != fImpl->fTerms.end(); ++t)
	{
		if (t->weight > maxTermFreq)
			maxTermFreq = t->weight;
//		
//		if (t->key.find('*') != string::npos or t->key.find('?') != string::npos)
//		{
//			inAllTermsRequired = false;
//			
//			vector<string> terms;
//			inDatabank.Expand(inIndex, t->key, terms);
//			
//			for (vector<string>::iterator et = terms.begin(); et != terms.end(); ++et)
//			{
//				auto_ptr<Term> nt(new Term);
//				nt->key = *et;
//				nt->weight = t->weight;
//				nt->w = 1.0f / terms.size();
//				expandedTerms.push_back(nt.release());
//			}
//			
//			t->weight = 0;
//		}		
	}
//	
//	if (expandedTerms.size())
//		fImpl->fTerms.insert(fImpl->fTerms.end(), expandedTerms.begin(), expandedTerms.end());

	// create the iterators and sort them by decreasing rank
	for (TermVector::iterator t = fImpl->fTerms.begin(); t != fImpl->fTerms.end(); ++t)
	{
		if (t->weight == 0)
			continue;
		
		t->weight = static_cast<uint32>(maxWeight * (static_cast<float>(t->weight) / maxTermFreq));
		
		t->iter.reset(inDatabank.GetDocWeightIterator(inIndex, t->key));
		if (t->iter.get() != nil)
		{
			if (t->w == 0)
				t->w = 1;
			t->w *= t->iter->Weight() * t->iter->GetIDFCorrectionFactor() * t->weight;
		}
		else if (not inAllTermsRequired or stopWords.count(t->key) == 1)
			t->w = 0;
		else		// short cut, no results
		{
			outCount = 0;
			return;
		}
	}
	
	fImpl->fTerms.sort(greater<Term>());

	CAccumulator A(inDatabank.Count());

	for (uint32 tx = 0; tx < fImpl->fTerms.size(); ++tx)
	{
		Term& t = fImpl->fTerms[tx];
		
		if (t.w == 0)	// done
			break;

		// use two thresholds, one to limit adding accumulators
		// and one to stop walking the iterator

		const float c_add = 0.007;
		const float c_ins = 0.12;

		float s_add = c_add * Smax;
		float s_ins = c_ins * Smax;

		if (t.iter.get() == nil)
			continue;

		++termCount;

		float idf = t.iter->GetIDFCorrectionFactor();
		float wq = idf * t.weight;

		Wq += wq * wq;

		uint32 docNr;
		uint8 rank;

		uint8 f_add = static_cast<uint8>(s_add / (idf * wq * wq));
		uint8 f_ins = static_cast<uint8>(s_ins / (idf * wq * wq));

		while (t.iter->Next(docNr, rank, false) and rank >= f_add)
		{
			if (rank >= f_ins or A[docNr] != 0)
			{
				float wd = rank;
				float sd = idf * wd * wq;

				Smax = max(Smax, A[docNr] += sd);
			}
		}
	}

	Wq = sqrt(Wq);

	vector<CDocScore> docs;
	docs.reserve(inMaxReturn);		// << return the top maxReturn documents
	
	if (not inAllTermsRequired)
		termCount = 0;
	
	auto_ptr<CDocIterator> rdi(new CAccumulator::Iterator(A, termCount, inMetaQuery != nil));

	if (inMetaQuery)
		rdi.reset(CDocIntersectionIterator::Create(
			new CSortDocIterator(rdi.release()), inMetaQuery));
	
	uint32 d = 0, n = 0;
	
	while (rdi->Next(d, false))
	{
		// skip over invalidated records
		if (not inDatabank.IsValidDocumentNr(d))
			continue;
		
		CDocScore ds;
		ds.fRank = alg(A[d], Wd[d], Wq);
		ds.fDocNr = d;
		
		if (n >= inMaxReturn)
		{
			if (ds.fRank > docs.front().fRank)
			{
				pop_heap(docs.begin(), docs.end(), greater<CDocScore>());
				docs.back() = ds;
				push_heap(docs.begin(), docs.end(), greater<CDocScore>());
			}
		}
		else
		{
			docs.push_back(ds);
			push_heap(docs.begin(), docs.end(), greater<CDocScore>());
		}

		++n;
	}

	outCount = n;
	
	sort_heap(docs.begin(), docs.end(), greater<CDocScore>());
	
	auto_ptr<CDocFreqVectorIterator::DocFreqVector> dv(new CDocFreqVectorIterator::DocFreqVector());
	dv->reserve(docs.size());

	for (vector<CDocScore>::iterator i = docs.begin(); i != docs.end(); ++i)
		dv->push_back(make_pair(i->fDocNr, i->fRank * 100));

	outResults.reset(new CDocFreqVectorIterator(dv.release()));
}
