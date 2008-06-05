/*	$Id: CLexicon.cpp 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Tuesday February 17 2004 10:41:58
	
	Store strings as densely packed as possible in order in which
	they are entered. Assign a sequence number to each entry and
	this number can be used later to retrieve the entry.
	
	This implementation used to use an AVL tree to store the lexical
	order. But I found out that a Patricia tree can reduce total
	indexing time by as much as 10% !!!
	
	Entries are never deleted so don't bother with that. Even worse,
	if you try to delete all Nodes, the indexing time will increase
	with 10%. So don't do it.
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

#include <boost/thread.hpp>

#include "HError.h"
#include "CLexicon.h"

using namespace std;

const int
	kLexDataSize = 8 * 1024 * 1024 - 3 * sizeof(uint32),	// 8 Mb per page
	kIxPageSize = 1024 * 1024;								// 1 million entries

struct CLexPage;

// CNode is a node in a Patricia tree

struct CNode
{
	CNode*			left;
	CNode*			right;
	uint32			value;
	uint32			bit;
};

// CLexPage stores strings without null terminator packed in s. 
// e is an array with offsets into s. Use negative offsets to
// access entries, this means s and e grow towards each other.
// This asserts that space is used optimally.
struct CLexPage
{
	int32		N;
	uint32		first;
	char		s[kLexDataSize];
	uint32		e[1];
	
				CLexPage(uint32 inFirst)
					: N(0)
					, first(inFirst)
				{
				}
				
	void		Add(const string& inString);

	inline
	string		GetEntry(uint32 inEntry) const
				{
					assert(static_cast<int32>(inEntry) < N);
					int32 ix = static_cast<int32>(inEntry);
					assert(e[-ix - 1] > e[-ix]);
					assert(e[-ix - 1] < kLexDataSize - N * sizeof(uint32));
					return string(s + e[-ix], e[-ix - 1] - e[-ix]);
				}

	inline
	int			Compare(const string& inKey, uint32 inEntry) const
				{
					assert(static_cast<int32>(inEntry) < N);
					int32 ix = static_cast<int32>(inEntry);
					uint32 l1 = inKey.length();
					uint32 l2 = e[-ix - 1] - e[-ix];
					
					uint32 l = l1;
					if (l > l2)
						l = l2;
					
					int d = strncmp(inKey.c_str(), s + e[-ix], l);
					if (d == 0)
						d = l1 - l2;

					return d;
				}

	inline
	int			Compare(const CLexPage* inPage, uint32 inPEntry, uint32 inEntry) const
				{
					assert(static_cast<int32>(inPEntry) < inPage->N);
					int32 ix1 = static_cast<int32>(inPEntry);
					uint32 l1 = inPage->e[-ix1 - 1] - inPage->e[-ix1];
					
					assert(static_cast<int32>(inEntry) < N);
					int32 ix2 = static_cast<int32>(inEntry);
					uint32 l2 = e[-ix2 - 1] - e[-ix2];
					
					uint32 l = l1;
					if (l > l2)
						l = l2;
					
					int d = strncmp(inPage->s + inPage->e[-ix1], s + e[-ix2], l);
					if (d == 0)
						d = l1 - l2;

					return d;
				}

	inline
	int			Compare(const CLexPage* inPage, uint32 inPEntry, uint32 inEntry, CLexCompare& inCompare) const
				{
					assert(static_cast<int32>(inPEntry) < inPage->N);
					int32 ix1 = static_cast<int32>(inPEntry);
					uint32 l1 = inPage->e[-ix1 - 1] - inPage->e[-ix1];
					
					assert(static_cast<int32>(inEntry) < N);
					int32 ix2 = static_cast<int32>(inEntry);
					uint32 l2 = e[-ix2 - 1] - e[-ix2];
					
					return inCompare.Compare(inPage->s + inPage->e[-ix1], l1, s + e[-ix2], l2);
				}

	inline
	bool		TestKeyBit(uint32 inEntry, uint32 inBit) const
				{
					assert(static_cast<int32>(inEntry) < N);
					int32 ix = static_cast<int32>(inEntry);
					assert(e[-ix - 1] > e[-ix]);
					assert(e[-ix - 1] < kLexDataSize - N * sizeof(uint32));
//					return string(s + e[-ix], e[-ix - 1] - e[-ix]);

					bool result = false;
					
					uint32 byte = inBit >> 3;
					if (byte < e[-ix - 1] - e[-ix])
					{
						uint32 bit = 7 - (inBit & 0x0007);
						result = (s[e[-ix] + byte] & (1 << bit)) != 0;
					}
					
					return result;
				}

	inline
	uint32		Free() const
				{
					return kLexDataSize - e[-static_cast<int32>(N)] - (N + 3) * sizeof(uint32);
				}
};

void CLexPage::Add(const string& inString)
{
	uint32 l = inString.length();
	
	assert(N == 0 or Free() >= l + sizeof(uint32));
	
	if (N == 0)
	{
		e[0] = 0;
		e[-1] = l;
		inString.copy(s, l);
		N = 1;
	}
	else
	{
		int32 nn = -static_cast<int32>(N);
		e[nn - 1] = e[nn] + l;
		inString.copy(s + e[nn], l);
		++N;
	}
}

struct CLexiconImp
{
	uint32			Store(const string& inWord);
	string			GetString(uint32 inNr) const;

	const CNode*	Find(const CNode* inNode, const string& inKey) const;
	bool			Find(const string& inKey, uint32& outNr) const;

	int				Compare(const string& inKey, uint32 inNr) const;
	int				Compare(const CNode* inA, const CNode* inB) const;
	int				Compare(uint32 inA, uint32 inB) const;
	int				Compare(uint32 inA, uint32 inB, CLexCompare& inCompare) const;

	inline
	bool			TestKeyBit(
						const char*		inKey,
						uint32			inKeyLength,
						uint32			inBit) const
					{
						bool result = false;
						
						uint32 byte = inBit >> 3;
						if (byte < inKeyLength)
						{
							uint32 bit = 7 - (inBit & 0x0007);
							result = (inKey[byte] & (1 << bit)) != 0;
						}
						
						return result;
					}

	inline
	bool			CompareKeyBits(
						const char*		inKeyA,
						uint32			inKeyALength,
						const char*		inKeyB,
						uint32			inKeyBLength,
						uint32			inBit) const
					{
						return
							TestKeyBit(inKeyA, inKeyALength, inBit) ==
							TestKeyBit(inKeyB, inKeyBLength, inBit);
					}

	bool			CompareKeyBits(
						const char*		inKey,
						uint32			inKeyLength,
						uint32			inNr,
						uint32			inBit) const;

					CLexiconImp();
					~CLexiconImp();

	typedef vector<CLexPage*> LexPageArray;

	LexPageArray::const_iterator
					GetPage(uint32& ioNr) const;

	typedef vector<CNode*> NodePageArray;

	CNode*			AllocateNode();

	LexPageArray	fPages;
	NodePageArray	fNodes;
	CNode*			fRoot;
	uint32			fCount;
	boost::mutex	fMutex;
};

CLexiconImp::CLexiconImp()
	: fCount(0)
{
	fPages.push_back(new CLexPage(0));

	fRoot = AllocateNode();
	
	fRoot->value = 0;
	fRoot->bit = 0;
	fRoot->left = fRoot;
	fRoot->right = fRoot;

	fPages.back()->Add(" ");	// avoid all kinds of checks below
	++fCount;
}

CLexiconImp::~CLexiconImp()
{
	for (LexPageArray::iterator p = fPages.begin(); p != fPages.end(); ++p)
		delete *p;

	for (NodePageArray::iterator p = fNodes.begin(); p != fNodes.end(); ++p)
		delete[] *p;
}

CNode* CLexiconImp::AllocateNode()
{
	uint32 ix = fCount % kIxPageSize;
	
	if (ix == 0)	// time for a new node page
	{
		assert((fCount / kIxPageSize) == fNodes.size());
		fNodes.push_back(new CNode[kIxPageSize]);
	}
	
	return fNodes.back() + ix;
}

inline
CLexiconImp::LexPageArray::const_iterator
CLexiconImp::GetPage(uint32& ioNr) const
{
	int32 L = 0, R = fPages.size() - 1;
	while (L <= R)
	{
		int32 i = (L + R) / 2;
		
		if (fPages[i]->first <= ioNr)
			L = i + 1;
		else
			R = i - 1;
	}
	
	LexPageArray::const_iterator p = fPages.begin() + L - 1;

	assert(L > 0);
	assert(uint32(L - 1) < fPages.size());
	assert((*p)->first <= ioNr);
	assert(ioNr < (*p)->first + (*p)->N);
	
	ioNr -= (*p)->first;
	
	return p;
}

const CNode* CLexiconImp::Find(const CNode* inNode, const string& inKey) const
{
	const CNode* p;
	
	do
	{
		p = inNode;
		
		if (TestKeyBit(inKey.c_str(), inKey.length(), inNode->bit))
			inNode = inNode->right;
		else
			inNode = inNode->left;
	}
	while (p->bit < inNode->bit);
	
	return inNode;
}

bool CLexiconImp::Find(const string& inKey, uint32& outNr) const
{
	bool result = false;
	
	if (fCount > 0)
	{
		const CNode* t = Find(fRoot, inKey);
		if (t != nil and Compare(inKey, t->value) == 0)
		{
			result = true;
			outNr = t->value;
		}
	}
	
	return result;
}

uint32 CLexiconImp::Store(const string& inKey)
{
	CNode* x = fRoot;

	const CNode* t = Find(x, inKey);

	if (t != fRoot and Compare(inKey, t->value) == 0)
		return t->value;

	uint32 i = 0;
	
	while (CompareKeyBits(inKey.c_str(), inKey.length(), t->value, i))
		++i;

	CNode* p;
	
	do
	{
		p = x;
		if (TestKeyBit(inKey.c_str(), inKey.length(), x->bit))
			x = x->right;
		else
			x = x->left;
	}
	while (x->bit < i and p->bit < x->bit);
	
	if (fPages.back()->Free() < inKey.length() + sizeof(uint32))
		fPages.push_back(new CLexPage(fCount));

	fPages.back()->Add(inKey);

	CNode* n = AllocateNode();
	n->value = fCount;
	n->bit = i;
	
	if (TestKeyBit(inKey.c_str(), inKey.length(), n->bit))
	{
		n->right = n;
		n->left = x;
	}
	else
	{
		n->right = x;
		n->left = n;
	}
	
	if (TestKeyBit(inKey.c_str(), inKey.length(), p->bit))
		p->right = n;
	else
		p->left = n;
	
	uint32 result = fCount;
	
	++fCount;
	
	return result;
}

inline string CLexiconImp::GetString(uint32 inNr) const
{
	LexPageArray::const_iterator p = GetPage(inNr);
	
	if (p == fPages.end() or (*p)->N <= 0 or inNr >= static_cast<uint32>((*p)->N))
		THROW(("Lexicon is invalid"));
	
	return (*p)->GetEntry(inNr);
}

int CLexiconImp::Compare(const string& inKey, uint32 inNr) const
{
	int result = 0;
	
	LexPageArray::const_iterator p = GetPage(inNr);
	
	if (p != fPages.end() and (*p)->N > 0 and inNr < static_cast<uint32>((*p)->N))
		result = (*p)->Compare(inKey, inNr);
	else
		assert(false);
	
	return result;
}

int CLexiconImp::Compare(const CNode* inA, const CNode* inB) const
{
	int result = 0;
	
	uint32 nrA = inA->value;
	uint32 nrB = inB->value;
	
	if (nrA != nrB)
	{
		LexPageArray::const_iterator a = GetPage(nrA);
		LexPageArray::const_iterator b = GetPage(nrB);
		
		assert(a != fPages.end());
		assert(b != fPages.end());
		
		if (a != fPages.end() and nrA < static_cast<uint32>((*a)->N) and
			b != fPages.end() and nrB < static_cast<uint32>((*b)->N))
		{
			result = (*b)->Compare(*a, nrA, nrB);
		}
	}
	
	return result;
}

int CLexiconImp::Compare(uint32 inA, uint32 inB) const
{
	int result = 0;
	
	if (inA != inB)
	{
		LexPageArray::const_iterator a = GetPage(inA);
		LexPageArray::const_iterator b = GetPage(inB);
		
		assert(a != fPages.end());
		assert(b != fPages.end());
	
		if (a != fPages.end() and inA < static_cast<uint32>((*a)->N) and
			b != fPages.end() and inB < static_cast<uint32>((*b)->N))
		{
			result = (*b)->Compare(*a, inA, inB);
		}
		else
			assert(false);
	}
	
	return result;
}

int CLexiconImp::Compare(uint32 inA, uint32 inB, CLexCompare& inCompare) const
{
	int result = 0;
	
	if (inA != inB)
	{
		LexPageArray::const_iterator a = GetPage(inA);
		LexPageArray::const_iterator b = GetPage(inB);
		
		assert(a != fPages.end());
		assert(b != fPages.end());
	
		if (a != fPages.end() and inA < static_cast<uint32>((*a)->N) and
			b != fPages.end() and inB < static_cast<uint32>((*b)->N))
		{
			result = (*b)->Compare(*a, inA, inB, inCompare);
		}
		else
			assert(false);
	}
	
	return result;
}

bool CLexiconImp::CompareKeyBits(
	const char*		inKey,
	uint32			inKeyLength,
	uint32			inNr,
	uint32			inBit) const
{
	bool b = false;
	
	LexPageArray::const_iterator p = GetPage(inNr);
	
	if (p != fPages.end() and (*p)->N > 0 and inNr < static_cast<uint32>((*p)->N))
		b = (*p)->TestKeyBit(inNr, inBit);
	else
		assert(false);
	
	return
		TestKeyBit(inKey, inKeyLength, inBit) == b;
}

CLexicon::CLexicon()
	: fImpl(new CLexiconImp)
{
}

CLexicon::~CLexicon()
{
	delete fImpl;
}

uint32 CLexicon::Store(const string& inWord)
{
	return fImpl->Store(inWord);
}

string CLexicon::GetString(uint32 inNr) const
{
	return fImpl->GetString(inNr);
}

int CLexicon::Compare(uint32 inA, uint32 inB) const
{
	return fImpl->Compare(inA, inB);
}

int CLexicon::Compare(uint32 inA, uint32 inB, CLexCompare& inCompare) const
{
	return fImpl->Compare(inA, inB, inCompare);
}

uint32 CLexicon::Count() const
{
	return fImpl->fCount;
}
