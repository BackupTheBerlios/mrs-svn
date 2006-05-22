/*	$Id: CDocWeightArrayImp.h 21 2006-03-09 13:41:27Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:41:32
*/

/*-
 * Copyright (c) 2005
 *      M.L. Hekkelman. All rights reserved.
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
 
#ifndef CDOCWEIGHTARRAY_H
#define CDOCWEIGHTARRAY_H

class HUrl;
class HFileStream;

class CDocWeightArray
{
  public:
						CDocWeightArray(HUrl& inFile, uint32 inCount);
						CDocWeightArray(HFileStream& inFile, int64 inOffset,
							uint32 inCount);
						CDocWeightArray(const CDocWeightArray& inOther);

						// this constructor creates a new array
						// concatenating the two arrays passed in
						CDocWeightArray(const CDocWeightArray& inOtherA,
							const CDocWeightArray& inOtherA);
	virtual				~CDocWeightArray();

	CDocWeightArray&	operator=(const CDocWeightArray& inOther);

	float				operator[](uint32 inDocNr) const;

  private:
  	struct CDocWeightArrayImp*	fImpl;
};

#endif
