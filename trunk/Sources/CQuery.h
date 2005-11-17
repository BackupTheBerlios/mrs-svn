/*	$Id: CQuery.h,v 1.16 2005/08/22 12:38:07 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Tuesday January 07 2003 20:12:37
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

#ifndef CQUERY_H
#define CQUERY_H

#include "HStlString.h"
#include "HStlVector.h"
#include "HStlLimits.h"

class CDatabankBase;

class CQuery
{
  public:
			CQuery(const std::string& inQuery, CDatabankBase& inDatabank,
				bool inAutoWildcard);
	virtual	~CQuery();
	
	bool	Next(uint32& outDoc);
	void	Skip(uint32 inNrOfDocs);
	void	Rewind();
	uint32	Count(bool inExact) const;
	
	void	GetAll(std::vector<uint32>& outDocNrs);
	
	CDatabankBase&
			GetDatabank();

  private:
			CQuery(const CQuery& inOther);
	CQuery&	operator=(const CQuery& inOther);

	struct CQueryImp*	fImpl;
};

//bool Query(const std::string& inQuery, CDatabankBase& inDatabank,
//	bool inAutoWildcard,
//	std::vector<uint32>& outDocs, uint32& outCount,
//	uint32 inFirstDoc = 0,
//	uint32 inMaxDocs = std::numeric_limits<uint32>::max());

#endif // CQUERY_H
