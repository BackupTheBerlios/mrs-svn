/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Friday May 27 2005 15:46:37
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
 
#ifndef CSEQUENCE_H
#define CSEQUENCE_H

#include <string>
#include "SmallObj.h"

struct CMatrix;

const char kAACodes[] =
{
	'L', 'A', 'G', 'S', 'V',
	'E', 'T', 'K', 'D', 'P',
	'I', 'R', 'N', 'Q', 'F',
	'Y', 'M', 'H', 'C', 'W',
	'B', 'Z', 'X', 'U', '-'
};

const uint32
	kAACodeCount = sizeof(kAACodes),
	kFilteredCode = 22,
	kUnknownCode = 23,
	kSentinalCode = 24,
	kSignalGapCode = 24;		// reuse of number

#if 0

typedef std::basic_string<uint8>	CSequence;

#else

typedef std::basic_string<uint8>	CMutableSequence;

const uint32
	kCSequenceImpPageSize = 1024;

class CSequence
{
  public:
							CSequence()
								: fData(nil)
							{
							}
							
							CSequence(uint32 inSize)
								: fData(new CSequenceImp(inSize))
							{
							}

							template<class InputIterator>
							CSequence(InputIterator inBegin, InputIterator inEnd)
								: fData(new CSequenceImp(inEnd - inBegin))
							{
								std::copy(inBegin, inEnd, fData->fPtr);
							}

							CSequence(const CSequence& inOther)
								: fData(inOther.fData)
							{
								if (fData != nil)
									fData->Reference();
							}

							CSequence(const uint8* inData, uint32 inLength)
								: fData(new CSequenceImp(inData, inLength))
							{
							}

	virtual					~CSequence()
							{
								if (fData != nil)
									fData->Release();
							}

	CSequence&				operator=(const CSequence& inOther)
							{
								if (fData != nil)
									fData->Release();
								fData = inOther.fData;
								if (fData != nil)
									fData->Reference();
								return *this;
							}
	
	const uint8*			c_str() const			{ return fData->fPtr; }
	uint32					length() const			{ return fData->fLength; }
	
	uint8					operator[](uint32 inIndex) const
													{ return fData->fPtr[inIndex]; }
	
//	typedef uint8*									iterator;
	typedef const uint8*							iterator;
	typedef const uint8*							const_iterator;
	
	typedef std::reverse_iterator<iterator>			reverse_iterator;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;
	
	iterator				begin()					{ return fData->fPtr; }
	const_iterator			begin() const			{ return fData->fPtr; }
	
	iterator				end()					{ return fData->fPtr + fData->fLength; }
	const_iterator			end() const				{ return fData->fPtr + fData->fLength; }

	reverse_iterator		rbegin()				{ return reverse_iterator(end()); }
	const_reverse_iterator	rbegin() const			{ return const_reverse_iterator(end()); }
	
	reverse_iterator		rend()					{ return reverse_iterator(begin()); }
	const_reverse_iterator	rend() const			{ return const_reverse_iterator(begin()); }

	CSequence				substr(uint32 inPos, uint32 inLength) const
													{ return CSequence(fData->fPtr + inPos, fData->fPtr + inPos + inLength); }

  private:
	
	struct CSequenceImp //: public Loki::SmallObject<DEFAULT_THREADING,kCSequenceImpPageSize>
	{
		uint8*				fPtr;
		uint32				fLength;
		uint32				fRefCount;
		bool				fReadOnly;
							
							CSequenceImp(uint32 inSize)
								: fPtr(NULL)
								, fLength(inSize)
								, fRefCount(1)
								, fReadOnly(false)
							{
								if (fLength)
									fPtr = new uint8[fLength];
							}

							CSequenceImp(const uint8* inData, uint32 inSize)
								: fPtr(const_cast<uint8*>(inData))
								, fLength(inSize)
								, fRefCount(1)
								, fReadOnly(true)
							{
							}

		void				Reference()
							{
								++fRefCount;
							}

		void				Release()
							{
								--fRefCount;
								if (fRefCount == 0)
									delete this;
							}

	  private:
		CSequenceImp&		operator=(const CSequenceImp&);
							CSequenceImp(const CSequenceImp&);

							~CSequenceImp()
							{
								assert(fRefCount == 0);
								if (not fReadOnly)
									delete[] fPtr;
							}
	};

	CSequenceImp*			fData;
};

#endif

uint8 Encode(char inAA);
CSequence Encode(const std::string& inAASequence);
std::string Decode(CSequence inSequence);

char Decode(uint8 inCode);

void MidLine(const CSequence& inQuery, const CSequence& inTarget,
	const CMatrix& inMatrix, uint32& outIdentity, uint32& outPositives,
	uint32& outGaps, std::string& outMidline);

void PrintAlignment(
	const CSequence& inQuery, uint32 inQueryStart,
	const CSequence& inTarget, uint32 inTargetStart);

// inlines

char inline Decode(uint8 inCode)
{
	char result = '_';
	if (inCode < kAACodeCount)
		result = kAACodes[inCode];
	return result;
}

#endif // CSEQUENCE_H
