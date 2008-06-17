/*	$Id: CDbInfo.cpp 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Friday March 05 2004 14:45:43
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

#include "HStream.h"
#include "HUtils.h"

#include "CDbInfo.h"

using namespace std;

CDbInfo::CDbInfo()
{
}

CDbInfo::CDbInfo(HStreamBase& inInfo)
{
	HSwapStream<net_swapper> file(inInfo);
	
	try
	{
		uint32 size;
		
		size = inInfo.Size();
		
		while (file.Tell() < size)
		{
			uint32 k, s;
	
			file >> k >> s;
			
			HAutoBuf<char> b(new char[s]);
			file.Read(b.get(), s);
			
			CInfoRec r;
			r.fKind = k;
			r.fData.assign(b.get(), s);
			
			fInfo.push_back(r);
		}
	}
	catch (...)
	{
	}
}

bool CDbInfo::Next(uint32& ioCookie, std::string& outData,
	uint32& outKind, uint32 inFilter) const
{
	bool result = false;
	
	while (not result and ioCookie < fInfo.size())
	{
		if (inFilter == 0 or fInfo[ioCookie].fKind == inFilter)
		{
			result = true;
			outData = fInfo[ioCookie].fData;
			outKind = fInfo[ioCookie].fKind;
		}
		
		++ioCookie;
	}
	
	return result;
}

void CDbInfo::Add(uint32 inKind, std::string inData)
{
	CInfoRec r;
	r.fKind = inKind;
	r.fData = inData;
	fInfo.push_back(r);
}

void CDbInfo::Write(HStreamBase& inFile)
{
	HSwapStream<net_swapper> file(inFile);
	
	for (vector<CInfoRec>::iterator i = fInfo.begin(); i != fInfo.end(); ++i)
	{
		uint32 size = (*i).fData.length();
		file << (*i).fKind << size;
		file.Write((*i).fData.c_str(), (*i).fData.length());
	}
}
