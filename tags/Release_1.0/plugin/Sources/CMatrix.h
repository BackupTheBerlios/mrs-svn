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
 
#ifndef MATRIX_H
#define MATRIX_H

#include <cassert>

#include "CSequence.h"

const int16
	kSentinalScore = -9999;

struct CMatrix
{
					CMatrix(const std::string& inName, long inGapOpen, long inGapExtend);

	int32			Score(unsigned char inAACode1, unsigned char inAACode2) const;
	int32			Score(const CSequence& inA, const CSequence& inB, int32 inOpenCost, int32 inExtendCost) const;

	int32			operator()(unsigned char inAACode1, unsigned char inAACode2) const
						{ return Score(inAACode1, inAACode2); }

	struct stat_values
	{
		double		lambda;
		double		K;
		double		logK;
		double		H;
		double		alpha;
		double		beta;
	}				gapped, ungapped;
	int32			mLowestScore, mHighestScore;
	int16			mTableData[kAACodeCount * kAACodeCount];
	int16*			mTable[kAACodeCount];

  private:
					CMatrix(const CMatrix&);
	CMatrix&			operator=(const CMatrix&);

	void			Read(std::istream& inStream);
};

inline int32 CMatrix::Score(unsigned char inAACode1, unsigned char inAACode2) const
{
	using namespace std;
	
	assert(inAACode1 < kAACodeCount);
	assert(inAACode2 < kAACodeCount);
	
	return mTableData[inAACode1 * kAACodeCount + inAACode2];
	
//	int32 result = mLowestScore;
//	if (inAACode1 < kAACodeCount and inAACode2 < kAACodeCount)
//		result = mTableData[inAACode1 * kAACodeCount + inAACode2];
////		result = mTable[inAACode1][inAACode2];
//	return result;
}

class CPSSMatrix
{
  public:
					CPSSMatrix(const CSequence& inQuery, const CMatrix& inMatrix);
					~CPSSMatrix();

	int32			Score(uint32 inQueryOffset, unsigned char inTargetCode) const;
	
  private:
	typedef int16 PSScore[kAACodeCount];

	PSScore*		mScores;
	uint32			mScoreCount;
};

inline int32 CPSSMatrix::Score(uint32 inQueryOffset, unsigned char inTargetCode) const
{
	assert(inQueryOffset < mScoreCount);
	assert(inTargetCode < kAACodeCount);
	
	int32 result = kSentinalScore;
	if (inQueryOffset < mScoreCount and inTargetCode < kAACodeCount)
		result = mScores[inQueryOffset][inTargetCode];
	return result;
}

#endif
