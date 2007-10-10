/*	$Id: CBlast.cpp 327 2007-02-09 11:17:14Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Monday May 30 2005 15:51:00
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

#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <algorithm>
#include <iterator>

#include <boost/bind.hpp>

#include "HUtils.h"
#include "HError.h"
#include "HMutex.h"

#include "CQuery.h"
#include "CSequence.h"
#include "CNCBI.h"
#include "CMatrix.h"
#include "CDatabank.h"
#include "CFilter.h"
#include "CBlast.h"

#include "CThread.h"

using namespace std;

const uint32
	kUngappedDropOff = 7,
	kGappedDropOff = 15,
	kGappedDropOffFinal = 25,
	kTreshHold = 11,
	kGapTrigger = 22,
	kUnusedDiagonal = 0xF3F3F3F3;

const double
	kLn2 = log(2.);

// a handy template to calculate powers as consts

template<int n> inline uint32 powi(uint32 x) { return powi<n - 1>(x) * x; }
template<> inline uint32 powi<1>(uint32 x) { return x; }

const uint32 kBits = 5;

template<int n> inline uint64 mask() { return ~0ULL >> (64 - n * kBits); }

// 
template<int WordSize>
struct WordBase
{
	static const uint32 kMaxIndex;

	WordBase()
	{
		for (int i = 0; i < WordSize; ++i)
			data[i] = 0;
	}

	template<class ForwardIterator>
	WordBase(ForwardIterator inSeq)
	{
		for (int i = 0; i < WordSize; ++i)
			data[i] = *inSeq++;
	}
	
	unsigned char&	operator[](uint32 ix)
	{
		return data[ix];
	}
	
	bool	operator<(const WordBase& inOther) const
	{
		int32 diff = 0;
		for (int i = WordSize - 1; i >= 0 and diff == 0; --i)
			diff = data[i] - inOther.data[i];
		return diff < 0;
	}
	
	// The permutation iterator returns each Word permutation that
	// results in a total score against inWord larger than inTreshhold
	struct PermutationIterator
	{
	  public:
						PermutationIterator(WordBase& inWord, const CMatrix& inMatrix, int32 inTreshhold);
		
		bool			Next(WordBase& outWord, uint32& outIndex);
	
	  private:
		WordBase&		mWord;
		const CMatrix&	mMatrix;
		int32			mTreshhold;
		uint32			mIndex;
	};
	
	unsigned char	data[WordSize];
};

template<int WordSize>
const uint32 WordBase<WordSize>::kMaxIndex = mask<WordSize>();

template<int WordSize>
WordBase<WordSize>::PermutationIterator::PermutationIterator(
		WordBase& inWord, const CMatrix& inMatrix, int32 inTreshhold)
	: mWord(inWord)
	, mMatrix(inMatrix)
	, mTreshhold(inTreshhold)
	, mIndex(0)
{
}
		
template<int WordSize>
bool WordBase<WordSize>::PermutationIterator::Next(WordBase& outWord, uint32& outIndex)
{
	const uint32 kMaxIndex = powi<WordSize>(kAACodeCount);

	bool result = false;
	WordBase w;

	while (result == false and mIndex < kMaxIndex)
	{
		uint32 ix = mIndex;
		int32 score = 0;
		
		for (int i = WordSize - 1; i >= 0; --i)
		{
			w[i] = ix % kAACodeCount;
			ix /= kAACodeCount;
			
			score += mMatrix(mWord[i], w[i]);
		}
		
		if (score >= mTreshhold)
		{
			result = true;
			outWord = w;
			outIndex = 0;
			for (uint32 i = 0; i < WordSize; ++i)
				outIndex = outIndex << kBits | w[i];
		}

		++mIndex;
	}
	
	return result;
}

template<class charT, class traits, int WordSize>
basic_ostream<charT, traits>&
operator<<(basic_ostream<charT, traits>& os, const WordBase<WordSize>& word)
{
	os << '\'';
	for (int i = 0; i < WordSize; ++i)
		os << Decode(word.data[i]);
	os << '\'';
	return os;
}

const uint32 kInlineCount = 2;

template<int WordSize>
class WordHitIteratorBase
{
	struct Entry
	{
		uint16					mCount;
		uint16					mDataOffset;
		uint16					mInline[kInlineCount];
	};

  public:

	typedef WordBase<WordSize>					Word;
	typedef typename Word::PermutationIterator	WordPermutationIterator;

	struct WordHitIteratorStaticData
	{
								WordHitIteratorStaticData()
									: mLookup(NULL)
									, mOffsets(NULL)
								{
								}
		
		HAutoBuf<Entry>			mLookup;
		HAutoBuf<uint32>		mOffsets;
	};
	
								WordHitIteratorBase(WordHitIteratorStaticData& inStaticData);
	
	static void					Init(const CSequence& inQuery, const CMatrix& inMatrix,
									uint32 inTreshhold, WordHitIteratorStaticData& outStaticData);
	void						Reset(const CSequence& inSequence);
	
	bool						Next(uint32& outQueryOffset, uint32& outTargetOffset);
	
	uint32						Index() const		{ return mIndex; }

  private:

	CSequence::const_iterator	mSeqBegin;
	CSequence::const_iterator	mSeqCurrent;
	CSequence::const_iterator	mSeqEnd;
	Entry*						mLookup;
	uint32*						mOffsets;
	uint32						mIndex;
	Entry						mCurrent;
};

template<int WordSize>
WordHitIteratorBase<WordSize>::WordHitIteratorBase(
		WordHitIteratorStaticData& inStaticData)
	: mLookup(inStaticData.mLookup.get())
	, mOffsets(inStaticData.mOffsets.get())
{
}

template<int WordSize>
void WordHitIteratorBase<WordSize>::Init(const CSequence& inQuery, const CMatrix& inMatrix,
	uint32 inTreshhold, WordHitIteratorStaticData& outStaticData)
{
	uint32 N = Word::kMaxIndex;;
	
	vector<vector<uint32> > test(N);
	
	for (uint32 i = 0; i < inQuery.length() - WordSize + 1; ++i)
	{
		Word w(inQuery.begin() + i);
		
		WordPermutationIterator p(w, inMatrix, inTreshhold);
		Word sw;
		uint32 ix;

		while (p.Next(sw, ix))
			test[ix].push_back(i);
	}
	
	uint32 M = 0;
	for (uint32 i = 0; i < N; ++i)
		M += test[i].size();
	
	outStaticData.mLookup.reset(new Entry[N]);
	outStaticData.mOffsets.reset(new uint32[M]);

	uint32* data = &outStaticData.mOffsets[0];

	for (uint32 i = 0; i < N; ++i)
	{
		outStaticData.mLookup[i].mCount = test[i].size();
		outStaticData.mLookup[i].mDataOffset = data - &outStaticData.mOffsets[0];

		for (uint32 j = 0; j < outStaticData.mLookup[i].mCount; ++j)
		{
			if (j >= kInlineCount)
				*data++ = test[i][j];
			else
				outStaticData.mLookup[i].mInline[j] = test[i][j];
		}
	}
	
	assert(data < &outStaticData.mOffsets[0] + M);
}

template<int WordSize>
void WordHitIteratorBase<WordSize>::Reset(const CSequence& inSequence)
{
	mSeqBegin = mSeqCurrent = inSequence.begin();
	mSeqEnd = inSequence.end();
	
	mIndex = 0;
	mCurrent.mCount = 0;

	for (uint32 i = 0; i < WordSize - 1 and mSeqCurrent != mSeqEnd; ++i)
		mIndex = mIndex << kBits | *mSeqCurrent++;
}

template<int WordSize>
inline
bool WordHitIteratorBase<WordSize>::Next(uint32& outQueryOffset, uint32& outTargetOffset)
{
	const uint32 kMask = mask<WordSize - 1>();
	bool result = false;

	// here it is, the hottest loop in the entire application
	for (;;)
	{
		if (mCurrent.mCount-- > 0)
		{
			if (mCurrent.mCount >= kInlineCount)
				outQueryOffset = mOffsets[mCurrent.mDataOffset + mCurrent.mCount - kInlineCount];
			else
				outQueryOffset = mCurrent.mInline[mCurrent.mCount];
			
			outTargetOffset = mSeqCurrent - mSeqBegin - WordSize;
			result = true;
			break;
		}
		
		if (mSeqCurrent == mSeqEnd)
			break;
		
		mIndex = ((mIndex & kMask) << kBits) | *mSeqCurrent++;
		mCurrent = mLookup[mIndex];
	}

	return result;
}

struct Hsp
{
	uint32		mScore;
	uint32		mQuerySeed;
	CSequence	mTarget;
	uint32		mTargetSeed;
	uint32		mQueryStart;
	uint32		mQueryEnd;
	uint32		mTargetStart;
	uint32		mTargetEnd;
	CSequence	mAlignedQuery;
	string		mAlignedQueryString;
	CSequence	mAlignedTarget;
	bool		mGapped;

	string		mMidline;
	uint32		mGaps;
	uint32		mIdentity;
	uint32		mPositive;
	
	bool		operator<(const Hsp& inHsp) const
				{
					return mScore < inHsp.mScore;
				}
	
	bool		Overlaps(const Hsp& inOther) const
				{
					return
						mQueryEnd >= inOther.mQueryStart and mQueryStart <= inOther.mQueryEnd and
						mTargetEnd >= inOther.mTargetStart and mTargetStart <= inOther.mTargetEnd;
				}

	// and since gcc sucks big time:
	bool		operator>(const Hsp& inHsp) const
				{
					return mScore > inHsp.mScore;
				}

	void		CalculateMidline(
					const CSequence&	inUnfilteredQuery,
					const CMatrix&		inMatrix,
					bool				inFilter);
};

void Hsp::CalculateMidline(
	const CSequence&	inUnfilteredQuery,
	const CMatrix&		inMatrix,
	bool				inFilter)
{
	CSequence alignedQuery = mAlignedQuery;
	
	if (inFilter)
	{
		CSequence::const_iterator u = inUnfilteredQuery.begin() + mQueryStart;
		CSequence::const_iterator f = alignedQuery.begin();
		
		CMutableSequence n;
		vector<bool> ff;
		
		for (; f != alignedQuery.end(); ++f)
		{
			if (*f == kFilteredCode)
			{
				n += *u;
				ff.push_back(true);
			}
			else
			{
				n += *f;
				ff.push_back(false);
			}

			if (*f != kSignalGapCode)
				++u;
		}
		
		alignedQuery = CSequence(n.begin(), n.end());
		mAlignedQueryString = Decode(alignedQuery);
		
		uint32 c = 0;
		for (vector<bool>::iterator ffi = ff.begin(); ffi != ff.end(); ++ffi, ++c)
		{
			if (*ffi)
				mAlignedQueryString[c] = tolower(mAlignedQueryString[c]);
		}
	}
	else
		mAlignedQueryString = Decode(alignedQuery);
	
	CSequence alignedTarget = mAlignedTarget;

	MidLine(alignedQuery, alignedTarget, inMatrix, mIdentity, mPositive, mGaps, mMidline);
}

struct Hit
{
	uint32		mDocNr;
	string		mDocID;
	uint32		mTargetLength;

	void		AddHsp(const Hsp& inHsp)
				{
					bool found = false;
			
					for (vector<Hsp>::iterator hsp = mHsps.begin(); hsp != mHsps.end(); ++hsp)
					{
						if (inHsp.Overlaps(*hsp))
						{
							if (hsp->mScore < inHsp.mScore)
								*hsp = inHsp;
							found = true;
							break;
						}
					}
			
					if (not found)
						mHsps.push_back(inHsp);
				}
	
	void		Cleanup()
				{
					sort(mHsps.begin(), mHsps.end(), greater<Hsp>());
					
					for (vector<Hsp>::iterator a = mHsps.begin(); a != mHsps.end(); ++a)
					{
						vector<Hsp>::iterator b = a + 1;
						while (b != mHsps.end())
						{
							if (a->Overlaps(*b))
								mHsps.erase(b);
							else
								++b;
						}
					}
				}
	
	bool		operator<(const Hit& inHit) const
				{
					assert(mHsps.size());
					return mDocNr < inHit.mDocNr or (mDocNr == inHit.mDocNr and mHsps[0].mScore < inHit.mHsps[0].mScore);
				}

	double		Expect(int64 inSearchSpace, double inLambda, double inKappa) const
				{
					double result = numeric_limits<double>::max();
					if (mHsps.size() > 0)
					{
						double bitScore = trunc((inLambda * mHsps.front().mScore - inKappa) / kLn2);
						result = inSearchSpace / pow(2., bitScore);
					}
					return result;
				}
	
	vector<Hsp>	mHsps;
};

struct CompareHitsOnFirstHspScore
{
	typedef Hit first_argument_type;
	typedef Hit second_argument_type;
	
	// order our hits descending on score and ascending on id
	
	bool operator()(const Hit& inA, const Hit& inB) const
	{
		return inA.mHsps[0].mScore > inB.mHsps[0].mScore or
			(inA.mHsps[0].mScore == inB.mHsps[0].mScore and inA.mDocID < inB.mDocID);
	}
};

struct Diagonal
{
	Diagonal()
		: query(0)
		, target(0)
	{
	}
	
	Diagonal(int32 inQuery, int32 inTarget)
	{
		if (inQuery > inTarget)
		{
			target = 0;
			query = inQuery - inTarget;
		}
		else
		{
			query = 0;
			target = -(inTarget - inQuery);
		}
	}
	
	bool operator<(const Diagonal& inOther) const
	{
		return query < inOther.query or
			(query == inOther.query and target < inOther.target);
	}
	
	int32	query;
	int32	target;
};

struct DiagonalStartTable
{
	DiagonalStartTable()
		: mTable(NULL)
		, mTableLength(0)
		, mQueryLength(0)
		, mTargetLength(0)
	{}
	
	void Reset(uint32 inQueryLength, uint32 inTargetLength)
	{
		mQueryLength = inQueryLength;
		mTargetLength = inTargetLength;
		
		uint32 n = mQueryLength + mTargetLength + 1;
		if (n > mTableLength or mTable == NULL)
		{
			uint32 k = ((n / 10240) + 1) * 10240;
			int32* t = new int32[k];
			delete[] mTable;
			mTable = t;
			mTableLength = k;
		}

		memset(mTable, 0xF3, n * sizeof(int32));
	}
	
	~DiagonalStartTable()
	{
		delete[] mTable;
	}
	
	int32&	operator[](const Diagonal& inD)
	{
		return mTable[mTargetLength + inD.target + inD.query];
	}
	
	int32*	mTable;
	uint32	mTableLength;
	
	uint32	mQueryLength;
	uint32	mTargetLength;
};

class CBlastQueryBase
{
	friend struct	CBlastHspIterator;
	friend struct	CBlastHitIterator;
	
  public:
					CBlastQueryBase(vector<Hit>& inHits, HMutex& inLock,
						const CSequence& inQuery, const CMatrix& inMatrix,
						double inExpect, bool inGapped,
						uint32 inReportLimit);
	virtual			~CBlastQueryBase();
	
	virtual bool	Test(uint32 inDocNr, const CSequence& inTarget) = 0;
	int32			Extend(int32& ioQueryStart, const CSequence& inTarget,
						int32& ioTargetStart, int32& ioDistance);
					
	void			Cleanup();

	void			SetStatistics(int64 inDbLength, uint32 inDbCount);
	
	Hit&			GetHit(uint32 inHitNr)				{ assert(inHitNr >= 0 and inHitNr < mHits.size()); return mHits[inHitNr]; }
	
	void			JoinHits(CBlastQueryBase& inOther);
	void			SortHits(const CDatabankBase& inDb, int64 inSearchSpace);

	double			Kappa()								{ return mMatrix.gapped.K; }
	double			Lambda()							{ return mMatrix.gapped.lambda; }
	double			Entropy()							{ return mMatrix.gapped.H; }


  protected:

	struct Data
	{
					Data();
					~Data();
	
		void		Resize(uint32 inDimX, uint32 inDimY);
	
		int16		operator()(uint32 inI, uint32 inJ) const;
		int16&		operator()(uint32 inI, uint32 inJ);
		
		void		Print();
		
		int16*		mData;
		uint32		mDataLength;
		uint32		mDimX;
		uint32		mDimY;
	};

	struct RecordTraceBack
	{
					RecordTraceBack(Data& inTraceBack)
						: mTraceBack(inTraceBack)
					{
					}

		int16		operator()(int16 inB, int16 inIx, int16 inIy, uint32 inI, uint32 inJ)
					{
						int16 result;

						if (inB >= inIx and inB >= inIy)
						{
							result = inB;
							mTraceBack(inI, inJ) = 0;
						}
						else if (inIx >= inB and inIx >= inIy)
						{
							result = inIx;
							mTraceBack(inI, inJ) = 1;
						}
						else
						{
							result = inIy;
							mTraceBack(inI, inJ) = -1;
						}
						
						return result;
					}
		
		void		Set(uint32 inI, uint32 inJ, int16 inD)
					{
						mTraceBack(inI, inJ) = inD;
					}
	
		Data&		mTraceBack;
	};

	struct DiscardTraceBack
	{
		int16		operator()(int16 inB, int16 inIx, int16 inIy, uint32 /*inI*/, uint32 /*inJ*/) const
					{
						return max(max(inB, inIx), inIy);
					}

		void		Set(uint32 inI, uint32 inJ, int16 inD)
					{
					}
	};

	template<class Iterator1, class Iterator2, class TraceBack>
	int32			AlignGapped(
						Iterator1 inQueryBegin, Iterator1 inQueryEnd,
						Iterator2 inTargetBegin, Iterator2 inTargetEnd,
						TraceBack& inTraceBack, int32 inDropOff,
						uint32& outBestX, uint32& outBestY);

	int32			AlignGapped(uint32 inQuerySeed, const CSequence& inTarget,
						uint32 inTargetSeed, int32 inDropOff);

	int32			AlignGappedWithTraceBack(int32 inDropOff, Hsp& ioHsp);

	const CSequence&		mQuery;
	const CMatrix&			mMatrix;
//	PSSMatrix				mPSSMatrix;
	int32					mOpenCost;
	int32					mExtendCost;
	int32					mTreshhold;
	double					mExpect;
	bool					mGapped;
	int32					mS1;
	int32					mS2;
	int32					mXu;			// dropoff for ungapped alignment
	int32					mXg;			// dropoff for gapped alignment
	int32					mXgFinal;		// dropoff for final gapped alignment
	int32					mHitWindow;
	double					mCutOff;
	uint32					mReportLimit;
		
	Data	 				mB;
	Data					mIx;
	Data					mIy;
	DiagonalStartTable		mDiagonalTable;
	
	// reportable values
	uint32					mHitsToDb;
	uint32					mExtensions;
	uint32					mSuccessfulExtensions;
	uint32					mHspsBetterThan10;
	uint32					mSuccessfulGappedAlignments;
	uint32					mGappedAlignmentAttempts;
	uint32					mGapCount;
			
	HMutex&					mLock;
	vector<Hit>&			mHits;
};

template<int WordSize>
class CBlastQuery : public CBlastQueryBase
{
  public:

	typedef WordBase<WordSize>					Word;
	typedef WordHitIteratorBase<WordSize>		WordHitIterator;
	typedef typename Word::PermutationIterator	WordPermutationIterator;
	typedef typename WordHitIterator::WordHitIteratorStaticData
												WordHitIteratorStaticData;

					CBlastQuery(vector<Hit>& inHits, HMutex& inLock,
						const CSequence& inQuery, const CMatrix& inMatrix,
						WordHitIteratorStaticData& inWHISD, double inExpect, bool inGapped,
						uint32 inReportLimit);

	virtual bool	Test(uint32 inDocNr, const CSequence& inTarget);
	
  private:

	WordHitIterator	mWordHitIterator;
};

CBlastQueryBase::Data::Data()
	: mData(NULL)
	, mDataLength(0)
	, mDimX(0)
	, mDimY(0)
{
}

void CBlastQueryBase::Data::Resize(uint32 inDimX, uint32 inDimY)
{
	uint32 n = (inDimX + 1) * (inDimY + 1);
	if (mData == NULL or n > mDataLength)
	{
		const uint32 kDataBlockExtend = 102400;
		uint32 k = ((n / kDataBlockExtend) + 1) * kDataBlockExtend;
		
		int16* t = new int16[k];

		delete[] mData;

		mData = t;
		mDataLength = k;
	}

	mDimX = inDimX;
	mDimY = inDimY;

	mData[0] = 0;
}

inline CBlastQueryBase::Data::~Data()
{
	delete[] mData;
}

inline int16& CBlastQueryBase::Data::operator()(uint32 inI, uint32 inJ)
{
	assert((inI > 0 and inJ > 0) or (inI == 0 and inJ == 0));
	
	assert(inI <= mDimX);
	assert(inJ <= mDimY);
	return mData[inI * mDimY + inJ];
}

void CBlastQueryBase::Data::Print()
{
	for (uint32 x = 0; x < mDimX; ++x)
		cout << '\t' << x;
	cout << endl;

	for (uint32 y = 0; y < mDimY; ++y)
	{
		cout << y;
		for (uint32 x = 0; x < mDimX; ++x)
			cout << '\t' << mData[x * mDimY + y];
		cout << endl;
	}
	
	cout << endl;
}

CBlastQueryBase::CBlastQueryBase(
		vector<Hit>& inHits, HMutex& inLock,
		const CSequence& inQuery, const CMatrix& inMatrix,
		double inExpect, bool inGapped,
		uint32 inReportLimit)
	: mQuery(inQuery)
	, mMatrix(inMatrix)
//	, mDb(inDb)
//	, mPSSMatrix(inQuery, inMatrix)
	, mOpenCost(12)
	, mExtendCost(1)
	, mTreshhold(kTreshHold)
	, mExpect(inExpect)
	, mGapped(inGapped)
	, mS1(0)
	, mS2(0)
	, mHitWindow(40)
	, mReportLimit(inReportLimit)
	, mHitsToDb(0)
	, mExtensions(0)
	, mSuccessfulExtensions(0)
	, mHspsBetterThan10(0)
	, mSuccessfulGappedAlignments(0)
	, mGappedAlignmentAttempts(0)
	, mLock(inLock)
	, mHits(inHits)
{
	// calculate other statistics
	int32 queryLen = mQuery.length();

	if (queryLen >= numeric_limits<uint16>::max())
		THROW(("Query length cannot be larger than %ld, sorry.", numeric_limits<uint16>::max()));

	mXu =		static_cast<int32>(ceil((kLn2 * kUngappedDropOff) / mMatrix.ungapped.lambda));
	mXg =		static_cast<int32>((kLn2 * kGappedDropOff) / mMatrix.gapped.lambda);
	mXgFinal =	static_cast<int32>((kLn2 * kGappedDropOffFinal) / mMatrix.gapped.lambda);
	mS1 =		static_cast<int32>((kLn2 * kGapTrigger + mMatrix.ungapped.logK) / mMatrix.ungapped.lambda);
	mS2 =		0;	// yeah, that sucks... perhaps
}

CBlastQueryBase::~CBlastQueryBase()
{
}

int32 CBlastQueryBase::Extend(int32& ioQueryStart, const CSequence& inTarget, int32& ioTargetStart, int32& ioDistance)
{
	// use iterators
	CSequence::const_iterator ai = mQuery.begin() + ioQueryStart;
	CSequence::const_iterator bi = inTarget.begin() + ioTargetStart;
	
	int32 score = 0;
	for (int i = 0; i < ioDistance; ++i, ++ai, ++bi)
		score += mMatrix(*ai, *bi);
	
	// record start and stop positions for optimal score
	CSequence::const_iterator qe = ai;
	
	for (int32 test = score, n = min(mQuery.end() - ai, inTarget.end() - bi);
		 test >= score - mXu and n > 0;
		 --n, ++ai, ++bi)
	{
		test += mMatrix(*ai, *bi);
		
		if (test > score)
		{
			score = test;
			qe = ai;
		}
	}
	
	ai = mQuery.begin() + ioQueryStart - 1;
	bi = inTarget.begin() + ioTargetStart - 1;
	CSequence::const_iterator qs = ai + 1;

	for (int32 test = score, n = min(ioQueryStart, ioTargetStart);
		 test >= score - mXu and n > 0;
		 --n, --ai, --bi)
	{
		test += mMatrix(*ai, *bi);

		if (test > score)
		{
			score = test;
			qs = ai;
		}
	}
	
	int32 delta = ioQueryStart - (qs - mQuery.begin());
	ioQueryStart -= delta;
	ioTargetStart -= delta;
	ioDistance = qe - qs;
	
	return score;
}

template<class Iterator1, class Iterator2, class TraceBack>
int32 CBlastQueryBase::AlignGapped(
	Iterator1		inQueryBegin,
	Iterator1		inQueryEnd,
	Iterator2		inTargetBegin,
	Iterator2		inTargetEnd,
	TraceBack&		inTraceBack,
	int32			inDropOff,
	uint32&			outBestX,
	uint32&			outBestY)
{
	const CMatrix& s = mMatrix;	// for readability
	TraceBack& tb_max = inTraceBack;
	int32 d = mOpenCost;
	int32 e = mExtendCost;
	
	uint32 dimX = inQueryEnd - inQueryBegin;
	uint32 dimY = inTargetEnd - inTargetBegin;
	
	Data& B = mB;	B.Resize(dimX, dimY);
	Data& Ix = mIx;	Ix.Resize(dimX, dimY);
	Data& Iy = mIy;	Iy.Resize(dimX, dimY);
	
	int32 bestScore = 0;
	uint32 bestX;
	uint32 bestY;
	uint32 colStart = 1;
	uint32 lastColStart = 1;
	uint32 colEnd = dimY;
	
	// first column
	uint32 i = 1, j = 1;
	Iterator1 x = inQueryBegin;
	Iterator2 y = inTargetBegin;

	// first cell
	int16 Ix1 = kSentinalScore, Iy1 = kSentinalScore;

	// (1)
	int16 M = s(*x, *y);
	
	// (2)
	(void)tb_max(M, kSentinalScore, kSentinalScore, 1, 1);
	bestScore = B(1, 1) = M;
	bestX = bestY = 1;

	// (3)
	Ix(1, 1) = M - d;

	// (4)
	Iy(1, 1) = M - d;

	// remaining cells in the first column
	y = inTargetBegin + 1;
	Ix1 = kSentinalScore;
	M = kSentinalScore;

	for (j = 2; y != inTargetEnd; ++j, ++y)
	{
		Iy1 = Iy(i, j - 1);
		
		// (2)
		int16 Bij = B(i, j) = Iy1;
		tb_max.Set(i, j, -1);

		// (3)
		Ix(i, j) = kSentinalScore;
		
		// (4)
		Iy(i, j) = Iy1 - e;

		if (Bij < bestScore - inDropOff)
		{
			colEnd = j;
			break;
		}
	}

	// remaining columns
	++x;
	for (i = 2; x != inQueryEnd and colEnd >= colStart; ++i, ++x)
	{
		y = inTargetBegin + colStart - 1;
		uint32 newColStart = colStart;
		bool beforeFirstRow = true;
		
		for (j = colStart; y != inTargetEnd; ++j, ++y)
		{
			Ix1 = kSentinalScore;
			Iy1 = kSentinalScore;
			
			if (j < colEnd)
				Ix1 = Ix(i - 1, j);
			
			if (j > colStart)
				Iy1 = Iy(i, j - 1);
			
			// (1)
			if (j <= lastColStart or j > colEnd + 1)
				M = kSentinalScore;
			else
				M = B(i - 1, j - 1) + s(*x, *y);
			
			// (2)
			int16 Bij = B(i, j) = tb_max(M, Ix1, Iy1, i, j);

			// (3)
			Ix(i, j) = max(M - d, Ix1 - e);
			
			// (4)
			Iy(i, j) = max(M - d, Iy1 - e);

			if (Bij > bestScore)
			{
				bestScore = Bij;
				bestX = i;
				bestY = j;
				beforeFirstRow = false;
			}
			else if (Bij < bestScore - inDropOff)
			{
				if (beforeFirstRow)
				{
					newColStart = j;
					if (newColStart > colEnd)
						break;
				}
				else if (j >= bestY)
				{
					colEnd = j;
					break;
				}
			}
			else
				beforeFirstRow = false;
		}
		
		lastColStart = colStart;
		colStart = newColStart;
	}
	
	outBestY = bestY;
	outBestX = bestX;
	
	return bestScore;
}

int32 CBlastQueryBase::AlignGapped(uint32 inQuerySeed, const CSequence& inTarget,
	uint32 inTargetSeed, int32 inDropOff)
{
	int32 score;
	
	uint32 x, y;
	
	DiscardTraceBack tb;
	
	score = AlignGapped(
		mQuery.begin() + inQuerySeed + 1, mQuery.end(),
		inTarget.begin() + inTargetSeed + 1, inTarget.end(),
		tb, inDropOff, x, y);
	
	score += AlignGapped(
		mQuery.rbegin() + (mQuery.length() - inQuerySeed), mQuery.rend(),
		inTarget.rbegin() + (inTarget.length() - inTargetSeed), inTarget.rend(),
		tb, inDropOff, x, y);
	
	score += mMatrix(mQuery[inQuerySeed], inTarget[inTargetSeed]);
	
	return score;
}

int32 CBlastQueryBase::AlignGappedWithTraceBack(int32 inDropOff, Hsp& ioHsp)
{
	uint32 x, y;
	Data d;
	int32 score = 0;
	
	ioHsp.mGapped = false;

	CMutableSequence alignedQuery;
	CMutableSequence alignedTarget;

	// start with the part before the seed
	d.Resize(ioHsp.mQuerySeed + 1, ioHsp.mTargetSeed + 1);
	RecordTraceBack tbb(d);

	score = AlignGapped(
		mQuery.rbegin() + (mQuery.length() - ioHsp.mQuerySeed), mQuery.rend(),
		ioHsp.mTarget.rbegin() + (ioHsp.mTarget.length() - ioHsp.mTargetSeed), ioHsp.mTarget.rend(),
		tbb, inDropOff, x, y);
	ioHsp.mQueryStart = ioHsp.mQuerySeed - x;
	ioHsp.mTargetStart = ioHsp.mTargetSeed - y;
	
	CSequence::const_iterator qi = mQuery.begin() + ioHsp.mQuerySeed - x, qis = qi;
	CSequence::const_iterator si = ioHsp.mTarget.begin() + ioHsp.mTargetSeed - y, sis = si;
	
	uint32 qLen = 1;
	uint32 sLen = 1;
	
	while (x >= 1 or y >= 1)
	{
		if (x >= 1 and y >= 1 and d(x, y) == 0)
		{
			alignedQuery += *qi++;
			alignedTarget += *si++;
			--x;
			--y;
		}
		else if (y >= 1 and d(x, y) < 0)
		{
			alignedQuery += kSignalGapCode;
			alignedTarget += *si++;
			--y;
			ioHsp.mGapped = true;
		}
		else // if (x >= 1 and d(x, y) > 0)
		{
			alignedQuery += *qi++;
			alignedTarget += kSignalGapCode;
			--x;
			ioHsp.mGapped = true;
		}
	}
	
	qLen += qi - qis;
	sLen += si - sis;
	
	// the seed itself
	alignedQuery += mQuery[ioHsp.mQuerySeed];
	alignedTarget += ioHsp.mTarget[ioHsp.mTargetSeed];
	score += mMatrix(mQuery[ioHsp.mQuerySeed], ioHsp.mTarget[ioHsp.mTargetSeed]);

	// and the part after the seed
	d.Resize(mQuery.length() - ioHsp.mQuerySeed, ioHsp.mTarget.length() - ioHsp.mTargetSeed);
	RecordTraceBack tba(d);

	score += AlignGapped(
		mQuery.begin() + ioHsp.mQuerySeed + 1, mQuery.end(),
		ioHsp.mTarget.begin() + ioHsp.mTargetSeed + 1, ioHsp.mTarget.end(),
		tba, inDropOff, x, y);

	CSequence::const_reverse_iterator qri = mQuery.rbegin() + (mQuery.length() - ioHsp.mQuerySeed) - 1 - x, qris = qri;
	CSequence::const_reverse_iterator sri = ioHsp.mTarget.rbegin() + (ioHsp.mTarget.length() - ioHsp.mTargetSeed) - 1 - y, sris = sri;
	
	CMutableSequence q, s;
	
	while (x >= 1 or y >= 1)
	{
		if (x >= 1 and y >= 1 and d(x, y) == 0)
		{
			q += *qri++;
			s += *sri++;
			--x;
			--y;
		}
		else if (y >= 1 and d(x, y) < 0)
		{
			q += kSignalGapCode;
			s += *sri++;
			--y;
			ioHsp.mGapped = true;
		}
		else // if (x >= 1 and d(x, y) > 0)
		{
			q += *qri++;
			s += kSignalGapCode;
			--x;
			ioHsp.mGapped = true;
		}
	}

	reverse(q.begin(), q.end());
	reverse(s.begin(), s.end());
	
	alignedQuery += q;
	alignedTarget += s;
	
	qLen += qri - qris;
	sLen += sri - sris;
	
	ioHsp.mAlignedQuery = CSequence(alignedQuery.begin(), alignedQuery.end());
	ioHsp.mAlignedTarget = CSequence(alignedTarget.begin(), alignedTarget.end());
	ioHsp.mQueryEnd = ioHsp.mQueryStart + qLen;
	ioHsp.mTargetEnd = ioHsp.mTargetStart + sLen;
	
	return score;
}

void CBlastQueryBase::JoinHits(CBlastQueryBase& inOther)
{
//	mHits.insert(mHits.end(), inOther.mHits.begin(), inOther.mHits.end());
}

void CBlastQueryBase::SortHits(const CDatabankBase& inDb, int64 inSearchSpace)
{
	for (vector<Hit>::iterator hit = mHits.begin(); hit != mHits.end(); ++hit)
		hit->mDocID = inDb.GetDocumentID(hit->mDocNr);

	sort(mHits.begin(), mHits.end(), CompareHitsOnFirstHspScore());

	mHits.erase(remove_if(mHits.begin(), mHits.end(),
		boost::bind(&Hit::Expect, _1, inSearchSpace, mMatrix.gapped.lambda, mMatrix.gapped.logK) > mExpect), mHits.end());
}

void CBlastQueryBase::Cleanup()
{
	StMutex lock(mLock);

	mGapCount = 0;

	for (vector<Hit>::iterator hit = mHits.begin(); hit != mHits.end(); ++hit)
	{
		for (vector<Hsp>::iterator hsp = hit->mHsps.begin(); hsp != hit->mHsps.end(); ++hsp)
		{
			if (mGapped)
			{
				uint32 newScore = AlignGappedWithTraceBack(mXgFinal, *hsp);
				
				assert(hsp->mAlignedQuery.length() == hsp->mAlignedTarget.length());
				
				if (newScore >= hsp->mScore)
					hsp->mScore = newScore;
	
				if (hsp->mGapped)
					++mGapCount;
			}
			else
			{
				hsp->mAlignedQuery = mQuery.substr(hsp->mQueryStart, hsp->mQueryEnd - hsp->mQueryStart);
				hsp->mAlignedTarget = hsp->mTarget.substr(hsp->mTargetStart, hsp->mTargetEnd - hsp->mTargetStart);
			}
		}
		
		hit->Cleanup();
	}
}

template<int WordSize>
CBlastQuery<WordSize>::CBlastQuery(
		vector<Hit>& inHits, HMutex& inLock, const CSequence& inQuery, const CMatrix& inMatrix,
		WordHitIteratorStaticData& inWHISD, double inExpect, bool inGapped, uint32 inReportLimit)
	: CBlastQueryBase(inHits, inLock, inQuery, inMatrix, inExpect, inGapped, inReportLimit)
	, mWordHitIterator(inWHISD)
{
}

template<int WordSize>
bool CBlastQuery<WordSize>::Test(uint32 inDocNr, const CSequence& inTarget)
{
	bool result = false;

	// short cut
	if (inTarget.length() < 2 * WordSize)
		return result;
	
	mDiagonalTable.Reset(mQuery.length(), inTarget.length());

	mWordHitIterator.Reset(inTarget);
	uint32 queryOffset, targetOffset;
	Hit hit;
	
	while (mWordHitIterator.Next(queryOffset, targetOffset))
	{
		++mHitsToDb;

		Diagonal d(queryOffset, targetOffset);
		int32 m = mDiagonalTable[d];

		int32 distance = queryOffset - m;
		
		if (m == int32(kUnusedDiagonal) or distance >= mHitWindow)
			mDiagonalTable[d] = queryOffset;
		else if (distance >= WordSize)
		{
			++mExtensions;
			
			int32 queryStart = m;
			int32 targetStart = targetOffset - distance;
			int32 alignmentDistance = distance + WordSize;
			
			if (targetStart < 0 or queryStart < 0)
				continue;

			int32 score = Extend(queryStart, inTarget, targetStart, alignmentDistance);

			if (score >= mS1)
			{
				++mSuccessfulExtensions;

				// perform a gapped alignment
				int32 querySeed = queryStart + alignmentDistance / 2;
				int32 targetSeed = targetStart + alignmentDistance / 2;

				if (mGapped and score < mS2)
				{
					++mGappedAlignmentAttempts;

					int32 gappedScore = AlignGapped(querySeed, inTarget, targetSeed, mXg);

					if (gappedScore > score)
					{
						++mSuccessfulGappedAlignments;
						score = gappedScore;
					}
				}
				
				if (score >= mS2)
				{
					Hsp hsp;
					hsp.mScore = score;
					hsp.mTarget = inTarget;
					hsp.mTargetSeed = targetSeed;
					hsp.mQuerySeed = querySeed;

					// this is of course not the exact data:
					hsp.mQueryStart = queryStart;
					hsp.mQueryEnd = queryStart + alignmentDistance;
					hsp.mTargetStart = targetStart;
					hsp.mTargetEnd = targetStart + alignmentDistance;

					hit.AddHsp(hsp);
				}
			}
			
			mDiagonalTable[d] = queryStart + alignmentDistance;
		}
	}
	
	if (hit.mHsps.size())
	{
		hit.mDocNr = inDocNr;
		hit.mTargetLength = inTarget.length();
		hit.Cleanup();
		
		StMutex lock(mLock);

		mHits.push_back(hit);
		
		// store at most mReportLimit hits. 
		CompareHitsOnFirstHspScore cmp;
		
		push_heap(mHits.begin(), mHits.end(), cmp);
		
		if (mHits.size() > mReportLimit)
		{
			pop_heap(mHits.begin(), mHits.end(), cmp);

			// we can now update mS2 to speed up things up a bit
			mS2 = mHits.front().mHsps.front().mScore;

			mHits.erase(mHits.end() - 1);
		}
	}
	
	return result;
}

class CBlastThread : public CThread
{
  public:
								CBlastThread(CBlastQueryBase& inBlastQuery, CDatabankBase& inDb,
										CDocIterator& inIter, HMutex& inMutex)
									: mBlastQuery(inBlastQuery)
									, mIter(inIter)
									, mDb(inDb)
									, mDbCount(0)
									, mDbLength(0)
									, mLock(inMutex)
								{
								}

	uint32						DbCount() const			{ return mDbCount; }
	int64						DbLength() const		{ return mDbLength; }

	const string&				Error() const			{ return mError; }

  protected:

	virtual void				Run();
	
  private:

	bool						Next(uint32& outDocNr, vector<CSequence>& outTargets);

	CBlastQueryBase&			mBlastQuery;
	CDocIterator&				mIter;
	CDatabankBase&				mDb;
	uint32						mDbCount;
	int64						mDbLength;
	HMutex&						mLock;
	string						mError;
};

void CBlastThread::Run()
{
	try
	{
		uint32 docNr;
		vector<CSequence> targets;

		while (Next(docNr, targets))
		{
			for (vector<CSequence>::iterator t = targets.begin(); t != targets.end(); ++t)
				mBlastQuery.Test(docNr, *t);
			
			targets.clear();
		}
	}
	catch (const exception& e)
	{
		cerr << "Exception catched in CBlastThread::Run \"" << e.what() << "\" exiting" << endl;
		mError = e.what();
	}
	catch (...)
	{
		cerr << "Exception catched in CBlastThread::Run, exiting" << endl;
		mError = "unknown exception";
	}
}

bool CBlastThread::Next(uint32& outDocNr, vector<CSequence>& outTargets)
{
	bool result = false;

	StMutex lock(mLock);

	while (mIter.Next(outDocNr, false))
	{
		uint32 seqCount = mDb.CountSequencesForDocument(outDocNr);
		
		if (seqCount == 0)
			continue;
		
		for (uint32 s = 0; s < seqCount; ++s)
		{
			CSequence seq;
			mDb.GetSequence(outDocNr, s, seq);
			
			mDbCount += 1;
			mDbLength += seq.length();
			
			outTargets.push_back(seq);
		}
		
		result = true;
		break;
	}

	return result;
}

struct CBlastImp
{
								CBlastImp(const string& inQuery, const string& inMatrix,
									uint32 inWordSize, double inExpect, bool inFilter,
									bool inGapped, uint32 inGapOpen, uint32 inGapExtend,
									uint32 inReportLimit);
	
	bool						Find(CDatabankBase& inDb, CDocIterator& inIter);
	
	CSequence					mQuery;
	CSequence					mUnfilteredQuery;
	CMatrix						mMatrix;
	uint32						mReportLimit;
	CDatabankBase*				mDb;
	uint32						mDbCount;
	int64						mDbLength;
	int64						mSearchSpace;
	auto_ptr<CBlastQueryBase>	mBlastQuery;
	uint32						mWordSize;
	bool						mFilter;
	bool						mGapped;
	double						mExpect;
	vector<Hit>					mHits;
	static HMutex				sLock;
};

HMutex CBlastImp::sLock;

CBlastImp::CBlastImp(const string& inQuery, const string& inMatrix, uint32 inWordSize,
		double inExpect, bool inFilter, bool inGapped, uint32 inGapOpen, uint32 inGapExtend, uint32 inReportLimit)
	: mMatrix(inMatrix, inGapOpen, inGapExtend)
	, mReportLimit(inReportLimit)
	, mWordSize(inWordSize)
	, mFilter(inFilter)
	, mGapped(inGapped)
	, mReportLimit(inReportLimit)
	, mExpect(inExpect)
{
	mUnfilteredQuery = Encode(inQuery);
	
	if (mFilter)
		mQuery = CSequence(Encode(SEG(inQuery)));
	else
		mQuery = mUnfilteredQuery;
}

bool CBlastImp::Find(CDatabankBase& inDb, CDocIterator& inIter)
{
	WordHitIteratorBase<2>::WordHitIteratorStaticData whiStaticData2;
	WordHitIteratorBase<3>::WordHitIteratorStaticData whiStaticData3;
	WordHitIteratorBase<4>::WordHitIteratorStaticData whiStaticData4;

	switch (mWordSize)
	{
		case 2:
			WordHitIteratorBase<2>::Init(mQuery, mMatrix, kTreshHold, whiStaticData2);
			
			mBlastQuery.reset(
				new CBlastQuery<2>(mHits, sLock, mQuery, mMatrix, whiStaticData2,
					mExpect, mGapped, mReportLimit));
			break;
		
		case 3:
			WordHitIteratorBase<3>::Init(mQuery, mMatrix, kTreshHold, whiStaticData3);
			
			mBlastQuery.reset(
				new CBlastQuery<3>(mHits, sLock, mQuery, mMatrix, whiStaticData3,
					mExpect, mGapped, mReportLimit));
			break;
		
		case 4:
			WordHitIteratorBase<4>::Init(mQuery, mMatrix, kTreshHold, whiStaticData4);
			
			mBlastQuery.reset(
				new CBlastQuery<4>(mHits, sLock, mQuery, mMatrix, whiStaticData4,
					mExpect, mGapped, mReportLimit));
			break;
		
		default:
			THROW(("Unsupported word size: %d", mWordSize));
	}

	mDb = &inDb;
	mDbCount = 0;
	mDbLength = 0;
	
	if (THREADS > 1)
	{
		HMutex lock;
		
		vector<CBlastQueryBase*> queries;
		vector<CBlastThread*> threads;
		for (uint32 n = 0; n < THREADS; ++n)
		{
			switch (mWordSize)
			{
				case 2:
					queries.push_back(
						new CBlastQuery<2>(mHits, sLock, mQuery, mMatrix, whiStaticData2,
							mExpect, mGapped, mReportLimit));
					break;
				
				case 3:
					queries.push_back(
						new CBlastQuery<3>(mHits, sLock, mQuery, mMatrix, whiStaticData3,
							mExpect, mGapped, mReportLimit));
					break;
				
				case 4:
					queries.push_back(
						new CBlastQuery<4>(mHits, sLock, mQuery, mMatrix, whiStaticData4,
							mExpect, mGapped, mReportLimit));
					break;
			}

			threads.push_back(new CBlastThread(*queries.back(), inDb, inIter, lock));
			threads.back()->Start();
		}
	
		string error;
	
		for (uint32 n = 0; n < THREADS; ++n)
		{
			threads[n]->Join();
			
			mDbCount += threads[n]->DbCount();
			mDbLength += threads[n]->DbLength();
			
			if (threads[n]->Error().length())
				error = threads[n]->Error();
			
			if (error.length() == 0)
				mBlastQuery->JoinHits(*queries[n]);
			
			delete threads[n];
			delete queries[n];
		}
		
		if (error.length())
			THROW((error.c_str()));
	}
	else
	{
		uint32 modulo = inDb.GetBlastDbCount() / 70;
		uint32 n = 0;
	
		uint32 docNr;
	
		while (inIter.Next(docNr, false))
		{
			if (VERBOSE and ++n % modulo == 0)
			{
				cerr << ".";
				cerr.flush();
			}
	
			CSequence target;
			
			uint32 seqCount = inDb.CountSequencesForDocument(docNr);
			for (uint32 s = 0; s < seqCount; ++s)
			{
				inDb.GetSequence(docNr, s, target);

				mDbCount += 1;
				mDbLength += target.length();

				mBlastQuery->Test(docNr, target);
			}
		}
	}

	int32 lengthAdjustment;
	(void)ncbi::BlastComputeLengthAdjustment(mMatrix.gapped.K,
		mMatrix.gapped.alpha / mMatrix.gapped.lambda, mMatrix.gapped.beta,
		mQuery.length(), mDbLength, mDbCount, lengthAdjustment);

	int64 effectiveQueryLength = mQuery.length() - lengthAdjustment;
	int64 effectiveDbLength = mDbLength - static_cast<int64>(mDbCount * lengthAdjustment);

	mSearchSpace = effectiveDbLength * effectiveQueryLength;

	mBlastQuery->Cleanup();
	mBlastQuery->SortHits(inDb, mSearchSpace);

	if (VERBOSE)
		cerr << endl;
	
	return true;
}

CBlast::CBlast(const string& inQuery, const string& inMatrix, uint32 inWordSize,
		double inExpect, bool inFilter, bool inGapped, uint32 inGapOpen, uint32 inGapExtend, uint32 inReportLimit)
	: mImpl(new CBlastImp(inQuery, inMatrix, inWordSize, inExpect, inFilter, inGapped, 
		inGapOpen, inGapExtend, inReportLimit))
{
}

CBlast::~CBlast()
{
	delete mImpl;
}

bool CBlast::Find(CDatabankBase& inDb, CDocIterator& inIter)
{
	return mImpl->Find(inDb, inIter);
}

unsigned long CBlast::DbCount()
{
	return mImpl->mDbCount;
}

unsigned long CBlast::DbLength()
{
	return mImpl->mDbLength;
}

unsigned long CBlast::EffectiveSpace()
{
	return mImpl->mSearchSpace;
}

double CBlast::Kappa()
{
	return mImpl->mBlastQuery->Kappa();
}

double CBlast::Lambda()
{
	return mImpl->mBlastQuery->Lambda();
}

double CBlast::Entropy()
{
	return mImpl->mBlastQuery->Entropy();
}

//	----------------------------------------------------------------------------
//  Result iterators
//	----------------------------------------------------------------------------

CBlastHitIterator CBlast::Hits()
{
	return CBlastHitIterator(this, mImpl->mBlastQuery.get());
}

CBlastHspIterator::CBlastHspIterator(const CBlastHspIterator& inOther)
	: mBlast(inOther.mBlast)
	, mBlastQuery(inOther.mBlastQuery)
	, mHit(inOther.mHit)
	, mHspNr(inOther.mHspNr)
{
}

CBlastHspIterator::CBlastHspIterator(CBlast* inBlast, CBlastQueryBase* inBQ, struct Hit* inHit)
	: mBlast(inBlast)
	, mBlastQuery(inBQ)
	, mHit(inHit)
	, mHspNr(-1)
{
}

CBlastHspIterator& CBlastHspIterator::operator=(const CBlastHspIterator& inOther)
{
	mBlast = inOther.mBlast;
	mBlastQuery = inOther.mBlastQuery;
	mHit = inOther.mHit;
	mHspNr = inOther.mHspNr;

	return *this;
}

bool CBlastHspIterator::Next()
{
	bool result = false;
	
	if (uint32(++mHspNr) < mHit->mHsps.size())
	{
		result = true;
		
		mHit->mHsps[mHspNr].CalculateMidline(
			mBlast->mImpl->mUnfilteredQuery, mBlastQuery->mMatrix, mBlast->mImpl->mFilter);
	}

	return result;
}

uint32 CBlastHspIterator::QueryStart()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mQueryStart;
}

uint32 CBlastHspIterator::SubjectStart()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mTargetStart;
}

uint32 CBlastHspIterator::SubjectLength()
{
	return mHit->mTargetLength;
}

string CBlastHspIterator::QueryAlignment()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mAlignedQueryString;
}

string CBlastHspIterator::SubjectAlignment()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return Decode(mHit->mHsps[mHspNr].mAlignedTarget);
}

string CBlastHspIterator::Midline()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mMidline;
}

uint32 CBlastHspIterator::Score()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mScore;
}

double CBlastHspIterator::BitScore()
{
	const CMatrix& m = mBlastQuery->mMatrix;
	return trunc((m.gapped.lambda * Score() - m.gapped.logK) / kLn2);
}

double CBlastHspIterator::Expect()
{
	return mBlast->mImpl->mSearchSpace / pow(2., BitScore());
}

uint32 CBlastHspIterator::Identity()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mIdentity;
}

uint32 CBlastHspIterator::Positive()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mPositive;
}

uint32 CBlastHspIterator::Gaps()
{
	assert(mHspNr >= 0 and uint32(mHspNr) < mHit->mHsps.size());
	return mHit->mHsps[mHspNr].mGaps;
}

CBlastHitIterator::CBlastHitIterator(const CBlastHitIterator& inOther)
	: mBlast(inOther.mBlast)
	, mBlastQuery(inOther.mBlastQuery)
	, mHitNr(inOther.mHitNr)
{
}

CBlastHitIterator::CBlastHitIterator(CBlast* inBlast, CBlastQueryBase* inBQ)
	: mBlast(inBlast)
	, mBlastQuery(inBQ)
	, mHitNr(-1)
{
}

CBlastHitIterator& CBlastHitIterator::operator=(const CBlastHitIterator& inOther)
{
	mBlast = inOther.mBlast;
	mBlastQuery = inOther.mBlastQuery;
	mHitNr = inOther.mHitNr;
	
	return *this;
}

bool CBlastHitIterator::Next()
{
	++mHitNr;
	return uint32(mHitNr) < mBlastQuery->mHits.size();
}

CBlastHspIterator CBlastHitIterator::Hsps()
{
	return CBlastHspIterator(mBlast, mBlastQuery, &mBlastQuery->GetHit(mHitNr));
}

uint32 CBlastHitIterator::DocumentNr() const
{
	return mBlastQuery->GetHit(mHitNr).mDocNr;
}

string CBlastHitIterator::DocumentID() const
{
	return mBlastQuery->GetHit(mHitNr).mDocID;
}
