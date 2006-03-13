/*	$Id$
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

#include "HStlVector.h"
#include "HUtils.h"
#include "HStlIOStream.h"

#include <boost/ptr_container/ptr_vector.hpp>

#include "CRankedQuery.h"
#include "CDatabank.h"
#include "CIndexer.h"

using namespace std;

struct Term
{
	string							key;
	float							weight;
	auto_ptr<CDbDocIteratorBase>	iter;
	float							w;
	
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

class CAccumulatorIterator : public CDocIterator
{
  public:
					CAccumulatorIterator(float inAccumulators[], uint32 inMaxDocCount)
						: fA(inAccumulators)
						, fCount(inMaxDocCount)
						, fPointer(0) {}
	
	virtual bool	Next(uint32& ioDoc, bool inSkip);
	virtual uint32	Count() const						{ return fCount; }
	virtual uint32	Read() const						{ return fPointer; }

  private:
	float*			fA;
	uint32			fCount;
	uint32			fPointer;
};

bool CAccumulatorIterator::Next(uint32& ioDoc, bool inSkip)
{
	bool done = false;
	
	if (inSkip)
		fPointer = ioDoc + 1;
	
	while (not done and fPointer < fCount)
	{
		if (fA[fPointer] != 0)
		{
			ioDoc = fPointer;
			done = true;
		}
		
		++fPointer;
	}
	
	return done;
}

CRankedQuery::CRankedQuery()
	: fImpl(new CRankedQueryImp)
{
}

CRankedQuery::~CRankedQuery()
{
	delete fImpl;
}

void CRankedQuery::AddTerm(const string& inKey, float inWeight)
{
	auto_ptr<Term> t(new Term);

	t->key = inKey;
	t->weight = inWeight;
	
	fImpl->fTerms.push_back(t.release());
}

CDocIterator* CRankedQuery::PerformSearch(CDatabankBase& inDatabank,
	const string& inIndex, CDocIterator* inMetaQuery)
{
	uint32 maxReturn = 1000;

	uint32 docCount = inDatabank.Count();
	HAutoBuf<float> Abuffer(new float[docCount]);
	float* A = Abuffer.get();		// the accumulators
	
	memset(A, 0, sizeof(float) * docCount);
	
	const CDocWeightArray& Wd = inDatabank.GetDocWeights(inIndex);
	
	float Wq = 0, Smax = 0;

	// first create the iterators and sort them by decreasing rank
	for (uint32 tx = 0; tx < fImpl->fTerms.size(); ++tx)
	{
		Term& t = fImpl->fTerms[tx];
		
		t.iter.reset(inDatabank.GetDocWeightIterator(inIndex, t.key));
		if (t.iter.get() != nil)
			t.w = t.iter->Weight() * t.iter->GetIDFCorrectionFactor() * t.weight;
		else
			t.w = 0;
	}
	
	fImpl->fTerms.sort(greater<Term>());

	for (uint32 tx = 0; tx < fImpl->fTerms.size(); ++tx)
	{
		Term& t = fImpl->fTerms[tx];

		if (t.iter.get() == nil)
			continue;

		float idf = t.iter->GetIDFCorrectionFactor();
		float wq = idf * t.weight;

		Wq += wq * wq;

		uint32 docNr;
		uint8 rank;

		// use two thresholds, one to limit adding accumulators
		// and one to stop walking the iterator

		const float c_add = 0.007;
		const float c_ins = 0.12;

		float s_add = c_add * Smax;
		float s_ins = c_ins * Smax;

		uint8 f_add = static_cast<uint8>(s_add / (idf * wq * wq));
		uint8 f_ins = static_cast<uint8>(s_ins / (idf * wq * wq));

		while (t.iter->Next(docNr, rank, false) and rank >= f_add)
		{
			if (rank >= f_ins or A[docNr] != 0)
			{
				float wd = rank;
				float sd = idf * wd * wq;

				A[docNr] += sd;
				Smax = max(Smax, A[docNr]);
			}
		}
	}
	
	Wq = sqrt(Wq);

	vector<CDocScore> docs;
	docs.reserve(maxReturn);		// << return the top maxReturn documents
	
	auto_ptr<CDocIterator> rdi(new CAccumulatorIterator(A, docCount));
	if (inMetaQuery)
		rdi.reset(new CDocIntersectionIterator(rdi.release(), inMetaQuery));
	
	uint32 d = 0;
	while (rdi->Next(d, false))
	{
		CDocScore ds;
		ds.fRank = A[d] / (Wd[d] * Wq);
		ds.fDocNr = d;
		
		if (docs.size() >= maxReturn)
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
	}
	
	sort_heap(docs.begin(), docs.end(), greater<CDocScore>());
	
	auto_ptr<CDocVectorIterator::DocFreqVector> dv(new CDocVectorIterator::DocFreqVector());
	dv->reserve(docs.size());

	for (vector<CDocScore>::iterator i = docs.begin(); i != docs.end(); ++i)
		dv->push_back(make_pair(i->fDocNr, static_cast<uint32>(i->fRank * 100)));

	return new CDocVectorIterator(dv.release());
}
