/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Friday April 02 2004 15:46:10
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

#include "CIdTable.h"
#include "CLexicon.h"
#include "CBitStream.h"

#include "HStream.h"
#include "CIndex.h"
#include <iostream>
#include <sstream>
#include "HUtils.h"
#include "HError.h"
#include "HByteSwap.h"

using namespace std;

const int
	kIdPageSize = 1024,
	kMaxDataOffset = kIdPageSize - 3 * sizeof(uint32);

struct CIdTablePage
{
	uint32		first;
	uint32		count;
	uint32		offset[1];
	char		data[kMaxDataOffset];
	
	void		NetToHost();
	void		HostToNet();
	
	string		GetID(uint32 inIndex) const;
	bool		Store(string inID);
};

string CIdTablePage::GetID(uint32 inIndex) const
{
	assert(inIndex < count);
	string result(data + offset[inIndex + 1], data + offset[inIndex]);
	return result;
}

void
CIdTablePage::NetToHost()
{
	first = net_swapper::swap(first);
	count = net_swapper::swap(count);
	
	for (uint32 i = 0; i <= count; ++i)
		offset[i] = net_swapper::swap(offset[i]);
}

void
CIdTablePage::HostToNet()
{
	for (uint32 i = 0; i <= count; ++i)
		offset[i] = net_swapper::swap(offset[i]);

	first = net_swapper::swap(first);
	count = net_swapper::swap(count);
}

bool CIdTablePage::Store(string inID)
{
	bool result = false;
	
	uint32 free = offset[count];
	assert(free <= kMaxDataOffset);
	free -= count * sizeof(uint32);
	
	if (free >= inID.length() + sizeof(uint32))
	{
		++count;
		offset[count] = offset[count - 1] - inID.length();
		assert(offset[count] >= count * sizeof(uint32));
		assert(offset[count] + inID.length() <= kIdPageSize - sizeof(uint32) * 3);
		inID.copy(data + offset[count], inID.length());
		result = true;
	}
	
	return result;
}

struct CIdTableImp
{
	HStreamBase*	fDataFile;
	int64			fOffset;
	int64			fSize;
	uint32			fPageCount;
};

CIdTable::CIdTable(HStreamBase& inFile, int64 inOffset, int64 inSize)
	: fImp(new CIdTableImp)
{
	fImp->fDataFile = &inFile;
	fImp->fOffset = inOffset;
	fImp->fSize = inSize;
	fImp->fPageCount = inSize / kIdPageSize;
	assert(fImp->fPageCount * kIdPageSize == inSize);
}

CIdTable::~CIdTable()
{
	delete fImp;
}

string CIdTable::GetID(uint32 inDocNr)
{
	int32 L = 0, R = fImp->fPageCount - 1;
	bool found = false;
	string result;
	
	while (not found and L <= R)
	{
		int32 i = (L + R) / 2;
		
		CIdTablePage p;
		
		int64 offset = fImp->fOffset + i * sizeof(p);
		
		fImp->fDataFile->PRead(&p, sizeof(p), offset);
		p.NetToHost();
		
		if (p.first > inDocNr)
			R = i - 1;
		else if (p.first + p.count <= inDocNr)
			L = i + 1;
		else
		{
			found = true;
			result = p.GetID(inDocNr - p.first);
		}
	}
	
	return result;
}

void CIdTable::Create(HStreamBase& inFile, CIteratorBase& inData,
	uint32 inDocCount)
{
	// Build the table in memory, let's pray it fits...
	
	try
	{
		CLexicon data;
		HAutoBuf<uint32> idMap(new uint32[inDocCount]);
		
		memset(idMap.get(), ~0, sizeof(uint32) * inDocCount);
		
		string id;
		int64 v;
		uint32 n = 0, i;
		
		while (inData.Next(id, v))
		{
			assert(v < inDocCount);
			
			if (v >= inDocCount)
				THROW(("Error creating ID table: v(%d) >= inDocCount(%d)", v, inDocCount));
			
			idMap[v] = data.Store(id);
			++n;
		}

		if (VERBOSE >= 1)
		{
			cout << "n: " << n << ", docs: " << inDocCount << endl;
			cout.flush();
		}

		if (n != inDocCount)
		{
			cerr << "Number of entries in the id index (" << n
				 << ") does not equal the number of documents (" << inDocCount
				 << "), this is an error" << endl;
			
			for (i = 0; i < inDocCount; ++i)
			{
				if (static_cast<int32>(idMap[i]) == -1)
				{
					stringstream s;
					s << '#' << i;
					idMap[i] = data.Store(s.str());
					
					cerr << "Adding id: " << s.str() << endl;
				}
			}
		}
		
		CIdTablePage p;
		p.first = 0;
		p.count = 0;
		p.offset[0] = kIdPageSize - sizeof(uint32) * 3;

		for (i = 0; i < n; ++i)
		{
			id = data.GetString(idMap[i]);
			if (not p.Store(id))
			{
				uint32 first = p.first + p.count;
				
				p.HostToNet();
				inFile.Write(&p, sizeof(p));

				p.first = first;
				p.count = 0;
				p.offset[0] = kIdPageSize - sizeof(uint32) * 3;
				bool b = p.Store(id);
				assert(b);
				(void)b;
			}
		}
		
		if (p.count > 0)
		{
			p.HostToNet();
			inFile.Write(&p, sizeof(p));
		}
	}
	catch (std::exception& e)
	{
		cerr << "Exception building ID table: " << e.what() << endl;
		throw;	// what else?
	}
}

void CIdTable::PrintInfo()
{
	cout << "ID Table contains " << fImp->fPageCount << " pages" << endl;
	cout << endl;
}
