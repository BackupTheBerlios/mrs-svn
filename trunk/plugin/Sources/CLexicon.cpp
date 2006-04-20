/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Tuesday February 17 2004 10:41:58
	
	Store strings as densely packed as possible in order in which
	they are entered. Assign a sequence number to each entry and
	this number can be used later to retrieve the entry.
	
	The sorted index on the data is stored as an AVL tree containing
	only the string numbers. Balance information is stored in the
	lower bits of the left, right pointers.
	
	Entries are never deleted so don't bother with that.'
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

#include "HError.h"
#include "CLexicon.h"

using namespace std;

#include <SmallObj.h>

const int
	kLexDataSize = 8 * 1024 * 1024 - 3 * sizeof(uint32),	// 8 Mb per page
	kIxPageSize = 1024 * 1024 * sizeof(void*);				// 4 or 8 Mb

struct CLexPage;

template <class T>
class CPointerPlusBit
{
  public:
			CPointerPlusBit(T* ptr = 0);
			CPointerPlusBit& operator=(const CPointerPlusBit& x);
			CPointerPlusBit& operator=(T* x);
			operator T*() const;
	bool	GetBit() const;
	void	SetBit(bool b);
	T*		operator->() const;

  private:
			CPointerPlusBit(const CPointerPlusBit&);

	T*		fPtr;
};

template <class T>
inline
CPointerPlusBit<T>::CPointerPlusBit(T* ptr)
	: fPtr(ptr)
{
}

template <class T>
inline
CPointerPlusBit<T>& CPointerPlusBit<T>::operator=(
	const CPointerPlusBit& x)
{
	fPtr = (T*)(((size_t)x.fPtr & ~1) | ((size_t)fPtr & 1));
	return *this;
}

template <class T>
inline
CPointerPlusBit<T>& CPointerPlusBit<T>::operator=(T* x)
{
	assert((((size_t)x) & 1) == 0);
	fPtr = (T*)((size_t)x | ((size_t)fPtr & 1));
	return *this;
}

template <class T>
inline
CPointerPlusBit<T>::operator T*() const
{
	return (T*)((size_t)fPtr & ~1);
}

template <class T>
inline
bool CPointerPlusBit<T>::GetBit() const
{
	return bool((size_t)fPtr & 1);
}

template <class T>
inline
void CPointerPlusBit<T>::SetBit(bool b)
{
	fPtr = (T*)(((size_t)fPtr & ~1) | (b & 1));
}

template <class T>
inline
T* CPointerPlusBit<T>::operator->() const
{
	return (T*)((size_t)fPtr & ~1);
}

enum Balance { Left, Even, Right };

struct CNode : public Loki::SmallObject<DEFAULT_THREADING, kIxPageSize>
{
	typedef CPointerPlusBit<CNode>	CNodePtr;

					CNode(uint32 inValue);
					~CNode();
	
	Balance			GetBalance() const;
	void			SetBalance(Balance inBalance);
	
	CNodePtr		left, right;
	uint32			value;
};

typedef CNode::CNodePtr CNodePtr;

CNode::CNode(uint32 inValue)
	: left(0)
	, right(0)
	, value(inValue)
{
}

CNode::~CNode()
{
	CNode* n = left;
	delete n;
	
	n = right;
	delete n;
}

inline
Balance CNode::GetBalance() const
{
	int r = 0;
	if (left.GetBit())
		r |= 1 << 1;
	if (right.GetBit())
		r |= 1 << 0;
	return Balance(r);
}

inline
void CNode::SetBalance(Balance inBalance)
{
	int b = int(inBalance);
	if (b & (1 << 1))
		left.SetBit(true);
	else
		left.SetBit(false);
	
	if (b & (1 << 0))
		right.SetBit(true);
	else
		right.SetBit(false);
}

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
					assert(inEntry < N);
					int32 ix = static_cast<int32>(inEntry);
					assert(e[-ix - 1] > e[-ix]);
					assert(e[-ix - 1] < kLexDataSize - N * sizeof(uint32));
					return string(s + e[-ix], e[-ix - 1] - e[-ix]);
				}

	inline
	int			Compare(const string& inKey, uint32 inEntry) const
				{
					assert(inEntry < N);
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
					assert(inPEntry < inPage->N);
					int32 ix1 = static_cast<int32>(inPEntry);
					uint32 l1 = inPage->e[-ix1 - 1] - inPage->e[-ix1];
					
					assert(inEntry < N);
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
	uint32		Free() const
				{
					return kLexDataSize - e[-N] - (N + 3) * sizeof(uint32);
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
		e[-N - 1] = e[-N] + l;
		inString.copy(s + e[-N], l);
		++N;
	}
}

struct CLexiconImp
{
	uint32			Store(const string& inWord);
	string			GetString(uint32 inNr) const;
	bool			Find(const string& inKey, uint32& outNr) const;
	void			Insert(CNode* inNode, CNodePtr& ioUnder, bool& outHeight);
	int				Compare(const string& inKey, uint32 inNr) const;
	int				Compare(const CNode* inA, const CNode* inB) const;
	int				Compare(uint32 inA, uint32 inB) const;

					CLexiconImp();
					~CLexiconImp();

	typedef vector<CLexPage*> LexPageArray;

	LexPageArray::const_iterator
					GetPage(uint32& ioNr) const;

	LexPageArray	fPages;
	CNodePtr		fRoot;
	uint32			fCount;
};

CLexiconImp::CLexiconImp()
	: fRoot(nil)
	, fCount(0)
{
	fPages.push_back(new CLexPage(0));
}

CLexiconImp::~CLexiconImp()
{
	for (LexPageArray::iterator p = fPages.begin(); p != fPages.end(); ++p)
		delete *p;

	// don't delete the nodes... that's not needed anyway
#if P_DEBUG
	CNode* n = fRoot;
	delete n;
#endif
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
	assert(L - 1 < fPages.size());
	assert((*p)->first <= ioNr);
	assert(ioNr < (*p)->first + (*p)->N);
	
	ioNr -= (*p)->first;
	
	return p;
}

bool CLexiconImp::Find(const string& inKey, uint32& outNr) const
{
	CNode* n = fRoot;
	bool result = false;
	
	while (result == false and n != nil)
	{
		int d = Compare(inKey, n->value);
		if (d < 0)
			n = n->left;
		else if (d > 0)
			n = n->right;
		else
		{
			result = true;
			outNr = n->value;
		}
	}
	
	return result;
}

uint32 CLexiconImp::Store(const string& inWord)
{
	uint32 result = 0;
	
	if (not Find(inWord, result))
	{
		if (fPages.back()->Free() < inWord.length() + sizeof(uint32))
			fPages.push_back(new CLexPage(fCount));

		fPages.back()->Add(inWord);
		
		result = fCount;
		
		bool h;
		Insert(new CNode(result), fRoot, h);
		
		++fCount;
	}
	
	return result;
}

void CLexiconImp::Insert(CNode* inNode, CNodePtr& p, bool& h)
{
	if (p == nil)
	{
		h = true;
		p = inNode;
		p->SetBalance(Even);
	}
	else
	{
		int d = Compare(inNode, p);
		if (d < 0)
		{
			Insert(inNode, p->left, h);
			if (h)
			{
				switch (p->GetBalance())
				{
					case Right:
						p->SetBalance(Even);
						h = false;
						break;
					case Even:
						p->SetBalance(Left);
						break;
					case Left:
					{
						CNode* p1 = p->left;
						if (p1->GetBalance() == Left)
						{
							p->left = p1->right;
							p1->right = p;
							p->SetBalance(Even);
							p = p1;
						}
						else
						{
							CNode* p2 = p1->right;
							p1->right = p2->left;
							p2->left = p1;
							p->left = p2->right;
							p2->right = p;
							if (p2->GetBalance() == Left)
								p->SetBalance(Right);
							else
								p->SetBalance(Even);
							if (p2->GetBalance() == Right)
								p1->SetBalance(Left);
							else
								p1->SetBalance(Even);
							p = p2;
						}
						p->SetBalance(Even);
						h = false;
						break;
					}
				}
			}
		}
		else if (d > 0)
		{
			Insert(inNode, p->right, h);
			if (h)
			{
				switch (p->GetBalance())
				{
					case Left:
						p->SetBalance(Even);
						h = false;
						break;
					case Even:
						p->SetBalance(Right);
						break;
					case Right: {
						CNode* p1 = p->right;
						if (p1->GetBalance() == Right)
						{
							p->right = p1->left;
							p1->left = p;
							p->SetBalance(Even);
							p = p1;
						}
						else
						{
							CNode* p2 = p1->left;
							p1->left = p2->right;
							p2->right = p1;
							p->right = p2->left;
							p2->left = p;
							if (p2->GetBalance() == Right)
								p->SetBalance(Left);
							else
								p->SetBalance(Even);
							if (p2->GetBalance() == Left)
								p1->SetBalance(Right);
							else
								p1->SetBalance(Even);
							p = p2;
						}
						p->SetBalance(Even);
						h = false;
						break;
					}
				}
			}
		}
		else
			assert(false);
	}
}

string CLexiconImp::GetString(uint32 inNr) const
{
	string result;
	
	LexPageArray::const_iterator p = GetPage(inNr);
	
	if (p != fPages.end() and inNr < (*p)->N)
		result = (*p)->GetEntry(inNr);
	else
		assert(false);
	
	return result;
}

int CLexiconImp::Compare(const string& inKey, uint32 inNr) const
{
	int result = 0;
	
	LexPageArray::const_iterator p = GetPage(inNr);
	
	if (p != fPages.end() and inNr < (*p)->N)
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
		
		if (a != fPages.end() and nrA < (*a)->N and
			b != fPages.end() and nrB < (*b)->N)
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
	
		if (a != fPages.end() and inA < (*a)->N and
			b != fPages.end() and inB < (*b)->N)
		{
			result = (*b)->Compare(*a, inA, inB);
		}
		else
			assert(false);
	}
	
	return result;
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

uint32 CLexicon::Count() const
{
	return fImpl->fCount;
}
