/*	$Id$
	Copyright hekkelman
	Created Monday February 20 2006 14:06:39
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
	docs.reserve(30);		// << return the top 30 documents
	
	auto_ptr<CDocIterator> rdi(new CAccumulatorIterator(A, docCount));
	if (inMetaQuery)
		rdi.reset(new CDocIntersectionIterator(rdi.release(), inMetaQuery));
	
	uint32 d = 0;
	while (rdi->Next(d, false))
	{
		CDocScore ds;
		ds.fRank = A[d] / (Wd[d] * Wq);
		ds.fDocNr = d;
		
		if (docs.size() >= 30)
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
