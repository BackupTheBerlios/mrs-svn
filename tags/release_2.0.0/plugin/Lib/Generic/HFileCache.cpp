/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday September 12 2000 11:10:51
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
 
#include "HLib.h"

#include <cstdio>
#include <cstdlib>

#include "HFileCache.h"
#include "HFile.h"
#include "HError.h"
#include "HUtils.h"

#define kErrIO "I/O Error in file cache (line %d)"

namespace HFileCache
{

using namespace std;

const int
	kCachedBlocks	= 128,
	kBlockSize		= 4096;

struct Entry
{
	int		fd;
	int32	pagesize;
	int64	offset;
	bool	dirty;
	Entry*	chain;		// hash
	Entry*	next;		// lru
	Entry*	prev;
	char*	data;

	static Entry*	sIndex;
	static uint32	sBlocks;
	static uint32	sBlockSize;
	static Entry*	sLRUHead;
	static Entry*	sLRUTail;
	static Entry**	sHashTable;
	static uint32	sBuckets;

	void			Read(int inFD, int64 inOffset);

//  public:

	static void		Init();
	static void		Cleanup();
	static uint32	Hash(int inFD, int64 inOffset);
	static Entry*	Lookup(int inFD, int64 inOffset);
	
	void			Flush();
	static void		FlushAll();
	static void		FlushFile(int inFD);
	void			Purge();
	static void		PurgeAll();
	static void		PurgeFile(int inFD);
	
	static void		TrucateFile(int inFD, int64 inSize);	
};

Entry*		Entry::sIndex = NULL;
uint32		Entry::sBlocks;
uint32		Entry::sBlockSize;
Entry*		Entry::sLRUHead;
Entry*		Entry::sLRUTail;
Entry**		Entry::sHashTable;
uint32		Entry::sBuckets;

void Entry::Init()
{
	if (sIndex == NULL)
	{
		sBlocks = kCachedBlocks;//gPrefs->GetPrefInt("file cache blocks", kCachedBlocks);
		sBlockSize = kBlockSize;
		
		const char* prefCacheBlocks = getenv("MRS_CACHE_BLOCK_COUNT");
		if (prefCacheBlocks)
			sBlocks = atoi(prefCacheBlocks);
		if (sBlocks < 8)
			THROW(("Error: cache block count too small (minimum is 8)"));
		
		const char* prefCacheBlockSize = getenv("MRS_CACHE_BLOCK_SIZE");
		if (prefCacheBlockSize)
			sBlockSize = atoi(prefCacheBlockSize);
		if (sBlockSize < 256)
			THROW(("Error: cache block count too small (minimum is 256)"));
		
		sIndex = new Entry[sBlocks];
		
		for (int i = 0; i < sBlocks; ++i)
		{
			sIndex[i].data = new char[sBlockSize];
			sIndex[i].fd = -1;
			sIndex[i].dirty = false;
			sIndex[i].chain = NULL;
			sIndex[i].next = sIndex + i + 1;
			sIndex[i].prev = sIndex + i - 1;
		}
		
		sIndex[0].prev = NULL;
		sIndex[sBlocks - 1].next = NULL;
		
		sLRUHead = sIndex;
		sLRUTail = sIndex + sBlocks - 1;
		
		sBuckets = 7 * sBlocks;	// arbitrary
		sHashTable = new Entry*[sBuckets];
		
		memset(sHashTable, 0, sizeof(Entry*) * sBuckets);
	}
}

void Entry::Cleanup()
{
	for (int i = 0; i < sBlocks; ++i)
		delete[] sIndex[i].data;
	
	delete[] sHashTable;
	sHashTable = NULL;

	delete[] sIndex;
	sIndex = NULL;
	
	sLRUHead = sLRUTail = NULL;
}

inline uint32 Entry::Hash(int inFD, int64 inOffset)
{
	return static_cast<uint32>((inFD * inOffset) % sBuckets);
}

Entry* Entry::Lookup(int inFD, int64 inOffset)
{
	uint32 hash = Hash(inFD, inOffset);
	
	Entry* result = sHashTable[hash];
	while (result != nil and (result->fd != inFD or result->offset != inOffset))
		result = result->chain;
	
	if (result == nil)
	{
		// not in cache, take the last used cache entry block
		result = sLRUTail;
		
		if (result == nil)
			THROW(("Cache error"));
		
		result->Read(inFD, inOffset);
	}
	
	// move the block to the front of the LRU list
	if (result != sLRUHead)
	{
		if (result == sLRUTail)
			sLRUTail = result->prev;
		
		if (result->prev)
			result->prev->next = result->next;
		if (result->next)
			result->next->prev = result->prev;

		result->prev = NULL;
		result->next = sLRUHead;
		sLRUHead->prev = result;
		sLRUHead = result;
	}
	
	return result;
}

void Entry::Read(int inFD, int64 inOffset)
{
	if (fd >= 0)
	{
		if (dirty)
			Flush();

		Purge();
	}
	
	fd = inFD;
	dirty = false;
	offset = inOffset;
	pagesize = HFile::PRead(inFD, data, sBlockSize, inOffset);

	if (pagesize < 0)
	{
		fd = -1;
		THROW((kErrIO, __LINE__));
	}
	
	// put into hash table
	uint32 hash = Hash(fd, offset);
	chain = sHashTable[hash];
	sHashTable[hash] = this;
}

void Entry::Flush()
{
	if (fd >= 0 and dirty)
	{
		int err = HFile::PWrite(fd, data, static_cast<uint32>(pagesize), offset);
		if (err < 0)
			THROW((kErrIO, __LINE__));
		dirty = false;
	}
}

void Entry::FlushAll()
{
	for (uint32 i = 0; i < sBlocks; ++i)
		if (sIndex[i].fd >= 0 and sIndex[i].dirty)
			sIndex[i].Flush();
}

void Entry::FlushFile(int inFD)
{
	for (int i = 0; i < sBlocks; ++i)
		if (sIndex[i].fd == inFD and sIndex[i].dirty)
			sIndex[i].Flush();
}

void Entry::Purge()
{
	uint32 hash = Hash(fd, offset);
	
	// remove old page from the hash table
	if (sHashTable[hash] == this)
		sHashTable[hash] = chain;
	else
	{
		Entry* e = sHashTable[hash];

		while (e != nil and e->chain != this)
			e = e->chain;

		if (e != nil)
			e->chain = chain;
	}
	
	fd = -1;
	offset = 0;
	chain = NULL;
}

void Entry::PurgeFile(int inFD)
{
	for (uint32 i = 0; i < sBlocks; ++i)
		if (sIndex[i].fd == inFD)
			sIndex[i].Purge();
}

void Entry::PurgeAll()
{
	for (uint32 i = 0; i < sBlocks; ++i)
		if (sIndex[i].fd >= 0)
			sIndex[i].Purge();
}

void Entry::TrucateFile(int inFD, int64 inSize)
{
	for (int i = 0; i < sBlocks; ++i)
	{
		if (sIndex[i].fd == inFD)
		{
			if (sIndex[i].offset > inSize)
				sIndex[i].Purge();
			else
			{
				sIndex[i].pagesize = static_cast<int32>(inSize - sIndex[i].offset);
				if (sIndex[i].pagesize > sBlockSize)
					sIndex[i].pagesize = sBlockSize;
				if (sIndex[i].pagesize > 0)
					sIndex[i].Flush();
			}
		}
	}
}

bool			sBypass = false;

void FlushBlock(int inIndex);

void Cleanup()
{
	Entry::Cleanup();
}

int32 Read(int inFD, void* inBuffer, uint32 inSize, int64 inOffset)
{
	if (sBypass)
		return HFile::PRead(inFD, inBuffer, inSize, inOffset);
	
	if (Entry::sIndex == NULL) Entry::Init();
	
	uint32 bOffset = 0;
	int32 rr = 0;
	
	while (inSize > 0)
	{
		int64 blockStart = (inOffset / Entry::sBlockSize) * Entry::sBlockSize;
		
		Entry* e = Entry::Lookup(inFD, blockStart);
		
		assert(e->pagesize >= (inOffset - blockStart));

		uint32 size = static_cast<uint32>(e->pagesize - (inOffset - blockStart));
		if (size > inSize)
			size = inSize;
		
		if (size == 0)
			break;

		rr += size;
		
		memcpy(reinterpret_cast<char*>(inBuffer) + bOffset,
			e->data + static_cast<uint32>(inOffset - blockStart), size);

		inSize -= size;
		inOffset += size;
		bOffset += size;
	}
	
	return rr;
}

char ReadChar(int inFD, int64 inOffset)
{
	if (Entry::sIndex == NULL) Entry::Init();
	
	int64 blockStart = (inOffset / Entry::sBlockSize) * Entry::sBlockSize;
	inOffset -= blockStart;
	
	Entry* e = Entry::Lookup(inFD, blockStart);
	return e->data[inOffset];
}

int32 Write(int inFD, const void* inBuffer, uint32 inSize, int64 inOffset)
{
	if (sBypass)
		return HFile::PWrite(inFD, inBuffer, inSize, inOffset);

	if (Entry::sIndex == NULL) Entry::Init();

	uint32 bOffset = 0;
	int32 rr = static_cast<int32>(inSize);	// for now...
	
	while (inSize > 0)
	{
		int64 blockStart = (inOffset / Entry::sBlockSize) * Entry::sBlockSize;

		Entry* e = Entry::Lookup(inFD, blockStart);
		e->dirty = true;

		if (e->pagesize < Entry::sBlockSize and 
			inOffset - blockStart + inSize > e->pagesize)
		{
			e->pagesize = static_cast<int32>(inOffset - blockStart + inSize);
			if (e->pagesize > Entry::sBlockSize)
				e->pagesize = Entry::sBlockSize;
		}
		uint32 size = static_cast<uint32>(e->pagesize - (inOffset - blockStart));
		if (size > inSize)
			size = inSize;
		if (size == 0)
			THROW((kErrIO, __LINE__));
		memcpy(e->data + static_cast<uint32>(inOffset - blockStart),
			reinterpret_cast<const char*>(inBuffer) + bOffset, size);
		inSize -= size;
		inOffset += static_cast<long>(size);
		bOffset += size;
	}
	
	return rr;
}

void Truncate(int inFD, int64 inSize)
{
	Entry::TrucateFile(inFD, inSize);
	HFile::Truncate(inFD, inSize);
}

void Flush()
{
	Entry::FlushAll();
}

void Flush(int inFD)
{
	Entry::FlushFile(inFD);
}

void Purge(int inFD)
{
	Entry::PurgeFile(inFD);
}

bool NextEOLN(int inFD, int64& ioOffset, uint32 inBlockSize, int ioState[4],
	const char* inEOLN[4], const int inEOLNLength[4])
{
	if (Entry::sIndex == NULL) Entry::Init();

	int64 bOffset = 0;
	
	while (inBlockSize > 0)
	{
		int64 blockStart = (ioOffset / Entry::sBlockSize) * Entry::sBlockSize;
		
		Entry* p = Entry::Lookup(inFD, blockStart);

		size_t size = static_cast<size_t>(p->pagesize - (ioOffset - blockStart));
		if (size > inBlockSize) size = inBlockSize;
		if (size == 0)
			break;
		
		for (int64 j = ioOffset - blockStart; j < ioOffset - blockStart + size; ++j)
		{
			for (int e = 0; e < 4; ++e)
			{
				if (inEOLNLength[e] == 0)
					continue;
				
				const char* eoln = inEOLN[e];

				if (p->data[j] != eoln[ioState[e]++])
					ioState[e] = 0;
				
				if (ioState[e] == inEOLNLength[e])
				{
					ioOffset = blockStart + j - inEOLNLength[e] + 1;
					return true;
				}
			}
		}
		
		inBlockSize -= size;
		ioOffset += static_cast<long>(size);
		bOffset += static_cast<long>(size);
	}
	return false;
}

bool NextEOLN(int inFD, int64& ioOffset, uint32 inBlockSize, int& ioState,
	const char* inEOLN, const int inEOLNLength)
{
	if (Entry::sIndex == NULL) Entry::Init();

	int64 bOffset = 0;
	
	while (inBlockSize > 0)
	{
		int64 blockStart = (ioOffset / Entry::sBlockSize) * Entry::sBlockSize;

		Entry* p = Entry::Lookup(inFD, blockStart);

		size_t size = static_cast<size_t>(p->pagesize - (ioOffset - blockStart));
		if (size > inBlockSize) size = inBlockSize;
		if (size == 0)
			break;
		
		for (int64 j = ioOffset - blockStart; j < ioOffset - blockStart + size; ++j)
		{
			if (p->data[j] != inEOLN[ioState++])
				ioState = 0;
			
			if (ioState == inEOLNLength)
			{
				ioOffset = blockStart + j - inEOLNLength + 1;
				return true;
			}
		}
		
		inBlockSize -= size;
		ioOffset += static_cast<long>(size);
		bOffset += static_cast<long>(size);
	}
	return false;
}

void BypassCache(bool inBypass)
{
	if (inBypass != sBypass)
	{
		if (inBypass)
		{
			Entry::FlushAll();
			Entry::PurgeAll();
		}
		
		sBypass = inBypass;
	}
}

}

