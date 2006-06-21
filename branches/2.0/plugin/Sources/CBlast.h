/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Monday May 30 2005 15:50:21
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
 
#ifndef CBLAST_H
#define CBLAST_H

#ifndef NO_BLAST

class CDatabank;
class CDocIterator;
class CBlastQueryBase;

struct CBlastHspIterator
{
	bool				Next();
	
	uint32				QueryStart();
	uint32				SubjectStart();
	
	std::string			QueryAlignment();
	std::string			SubjectAlignment();
	
	uint32				Score();
	double				BitScore();
	double				Expect();
	
//						CBlastHspIterator();
						CBlastHspIterator(const CBlastHspIterator&);
	CBlastHspIterator&	operator=(const CBlastHspIterator&);

						CBlastHspIterator(CBlastQueryBase* inBQ, const struct Hit* inHit);
	
  private:
	
	CBlastQueryBase*	mBlastQuery;
	const struct Hit*	mHit;
	int32				mHspNr;
};

struct CBlastHitIterator
{
	bool				Next();
	
	CBlastHspIterator	Hsps();
	uint32				DocumentNr() const;
	std::string			DocumentID() const;

//						CBlastHitIterator();
						CBlastHitIterator(const CBlastHitIterator&);
	CBlastHitIterator&	operator=(const CBlastHitIterator&);
					
  private:
	friend class		CBlast;

						CBlastHitIterator(CBlastQueryBase* inBQ);

	CBlastQueryBase*	mBlastQuery;
	int32				mHitNr;
};

class CBlast
{
  public:
						CBlast(const std::string& inQuery,
							const std::string& inMatrix, uint32 inWordSize,
							double inExpect,
							bool inFilter, bool inGapped,
							uint32 inGapOpen, uint32 inGapExtend);
	virtual				~CBlast();
	

	bool				Find(CDatabankBase& inDb, CDocIterator& inIter);
	
	std::string			ReportInXML();

	CBlastHitIterator	Hits();
	
  private:
	struct CBlastImp*	mImpl;
};


#endif // NO_BLAST

#endif // CBLAST_H
