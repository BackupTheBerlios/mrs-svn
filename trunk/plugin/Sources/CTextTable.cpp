/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Saturday January 11 2003 10:03:06
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

#include <algorithm>

#include "CTextTable.h"

using namespace std;

const int
	kSmallBufferSize = 0x010000,	// 65k
	kMinFreeSpaceInBlock = 8;		// If the free space is less than this tresh hold
									// we consider the block full


struct CTextTableBlock
{
					CTextTableBlock();
					~CTextTableBlock();

	const char*		store(const char* inText, uint32 inLength);
	uint32			free() const		{ return static_cast<uint32>(kSmallBufferSize - (end - text)); }

  private:

	char*			text;
	char*			end;
	
					CTextTableBlock(const CTextTableBlock&);
	CTextTableBlock&			operator=(const CTextTableBlock&);
};

CTextTableBlock::CTextTableBlock()
	: text(new char[kSmallBufferSize])
	, end(text)
{
}

CTextTableBlock::~CTextTableBlock()
{
	delete[] text;
}

const char* CTextTableBlock::store(const char* inText, uint32 inLength)
{
	assert(end + inLength <= text + kSmallBufferSize);
	
	const char* result = end;
	memcpy(end, inText, inLength);
	end += inLength;
	return result;
}

CTextTable::CTextTable()
	: size(0)
{
}

CTextTable::~CTextTable()
{
	Reset();
}

struct CompareBlocks
{
	bool operator()(const CTextTableBlock* a, const CTextTableBlock* b) const
		{ return a->free() < b->free(); }
};

const char* CTextTable::Store(const char* inText)
{
	uint32 n = strlen(inText) + 1;
	const char* result;

	if (free_blocks.size() == 0 || free_blocks.front()->free() < n)
	{
		free_blocks.push_back(new CTextTableBlock);
		push_heap(free_blocks.begin(), free_blocks.end(), CompareBlocks());
	}

	pop_heap(free_blocks.begin(), free_blocks.end(), CompareBlocks());
	
	result = free_blocks.back()->store(inText, n);
	
	if (free_blocks.back()->free() > kMinFreeSpaceInBlock)
		push_heap(free_blocks.begin(), free_blocks.end(), CompareBlocks());
	else
	{
		full_blocks.push_back(free_blocks.back());
		free_blocks.pop_back();
	}

	size += n - 1;
	
	return result;
}

void CTextTable::Reset()
{
	CBlockList::iterator i;
	
	for (i = free_blocks.begin(); i != free_blocks.end(); ++i)
		delete *i;
//	free_blocks = CBlockList();

	for (i = full_blocks.begin(); i != full_blocks.end(); ++i)
		delete *i;
//	full_blocks = CBlockList();
	
	size = 0;
}
