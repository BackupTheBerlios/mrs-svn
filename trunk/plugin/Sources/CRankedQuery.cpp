/*	$Id$
	Copyright hekkelman
	Created Monday February 20 2006 14:06:39
*/

#include "MRS.h"

#include "HStlVector.h"
#include "HUtils.h"
#include "HStlIOStream.h"

#include "CRankedQuery.h"
#include "CDatabank.h"
#include "CIndexer.h"

using namespace std;

typedef pair<string,float>	Term;
typedef vector<Term>		TermVector;

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
	fImpl->fTerms.push_back(make_pair(inKey, inWeight));
}

CDocIterator* CRankedQuery::PerformSearch(CDatabankBase& inDatabank, const string& inIndex)
{
	// eerste poging
	
	uint32 docCount = inDatabank.Count();
	HAutoBuf<float> Abuffer(new float[docCount]);
	float* A = Abuffer.get();		// the accumulators
	
	memset(A, 0, sizeof(float) * docCount);
	
	auto_ptr<CDocWeightArray> docWeights(inDatabank.GetDocWeights(inIndex));
	CDocWeightArray& Wd = *docWeights.get();
	
	float Wq = 0;

	for (uint32 tx = 0; tx < fImpl->fTerms.size(); ++tx)
	{
		Term t = fImpl->fTerms[tx];

		auto_ptr<CDbDocIteratorBase> iter(inDatabank.GetDocWeightIterator(inIndex, t.first));

		float idf = iter->GetIDFCorrectionFactor();
		float wq = idf * t.second;

		Wq += wq * wq;
		
		uint32 docNr;
		uint8 rank;
		
		while (iter->Next(docNr, rank, false))
		{
			float r = static_cast<float>(rank) / kMaxWeight;
			float wd = idf * r;
			A[docNr] += wd * wq;
		}
	}
	
	vector<CDocScore> docs;
	docs.reserve(30 + 1);		// << return the top 30 documents
	
	for (uint32 d = 0; d < docCount; ++d)
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
	
	auto_ptr<vector<uint32> > dv(new vector<uint32>());
	dv->reserve(docs.size());

#if P_DEBUG
	cout << endl;
#endif

	for (vector<CDocScore>::iterator i = docs.begin(); i != docs.end(); ++i)
	{
#if P_DEBUG
		cout << inDatabank.GetDocumentID(i->fDocNr) << ": " << i->fRank;

		cout << "\t" << inDatabank.GetDocument(i->fDocNr) << endl;
#endif
		dv->push_back(i->fDocNr);
	}

	return new CDocVectorIterator(dv.release());
}
