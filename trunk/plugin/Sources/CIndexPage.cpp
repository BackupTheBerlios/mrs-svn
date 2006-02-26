/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Monday December 09 2002 08:32:33
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

#include "HStlIOStream.h"
#include "HStlMap.h"
#include "HStlList.h"
#include "HStdCString.h"

#include "CIndexPage.h"
#include "HStream.h"
#include "CUtils.h"
#include "HUtils.h"
#include "HError.h"

using namespace std;

enum MatchResult {
	eMatch,
	eNoMatchAndLess,
	eNoMatchAndEqual,
	eNoMatchAndGreater
};

#if P_DEBUG

const char* MatchResultName[] = {
	"eMatch",
	"eNoMatchAndLess",
	"eNoMatchAndEqual",
	"eNoMatchAndGreater"
};

#endif

// ---------------------------------------------------------------------------
// 
// First define the traits for comparing key types and the Match routine for
// glob-like patterns
// 

static MatchResult Match(const char* inPattern, const char* inName)
{
	for (;;)
	{
		char op = *inPattern;

		switch (op)
		{
			case 0:
				if (*inName != 0)
					return eNoMatchAndGreater;
				else
					return eMatch;
			case '*':
			{
				if (inPattern[1] == 0)	// last '*' matches all 
					return eMatch;

				const char* n = inName;
				while (*n)
				{
					if (Match(inPattern + 1, n) == eMatch)
						return eMatch;
					++n;
				}
				return eNoMatchAndEqual;
			}
			case '?':
				if (*inName)
				{
					if (Match(inPattern + 1, inName + 1) == eMatch)
						return eMatch;
					else
						return eNoMatchAndEqual;
				}
				else
					return eNoMatchAndLess;
			default:
				if (tolower(*inName) == tolower(op))
				{
					++inName;
					++inPattern;
				}
				else
				{
					int d = tolower(op) - tolower(*inName);
					if (d > 0)
						return eNoMatchAndLess;
					else
						return eNoMatchAndGreater;
				}
				break;
		}
	}
}

int CompareKeyString(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB)
{
	int l = inLengthA;
	if (l > inLengthB)
		l = inLengthB;
	
	int d = strncasecmp(inA, inB, l);

	if (d == 0)
		d = inLengthA - inLengthB;
	
	return d;
}

int CompareKeyNumber(const char* inA, uint32 inLengthA, const char* inB, uint32 inLengthB)
{
	while (inLengthA > 0 and *inA == '0')
	{
		++inA;
		--inLengthA;
	}
	
	while (inLengthB > 0 and *inB == '0')
	{
		++inB;
		--inLengthB;
	}

	int d = inLengthA - inLengthB;
	
	while (d == 0 and inLengthA > 0)
	{
		d = *inA++ - *inB++;
		--inLengthA;
	}

	return d;
}

// ---------------------------------------------------------------------------
// 
//  The on disk data. Don't change this... it will break backwards compatibility
// 

const uint32
	kPageSize = 2048,
	kEntrySize = 2 * sizeof(uint32),
	kKeySpace = kPageSize - kEntrySize - 3 * sizeof(uint32),
	kMaxKeySize = (kKeySpace / 4 > 255 ? 255 : kKeySpace / 4);

struct COnDiskData
{
	unsigned char	keys[kKeySpace];
	struct
	{
		uint32		value;
		uint32		p;
	} e[1];
	uint32			p0;
	uint32			pp;
	int32			n;
};

class CIndexPage
{
  public:
							// first constructor, create a new page on disk
							CIndexPage(HStreamBase& inFile, int64 inBaseOffset);
							
							// second constructor, load the page from disk
							CIndexPage(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset);
							
							// default constructor, simply an empty page
							CIndexPage();
							
							// always flush the data if it was changed
							~CIndexPage();

							// load the page from disk
	void					Load(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset);

							// create a new page on disk
	void					Allocate();

	uint32					GetOffset() const	{ return fOffset; }

	const COnDiskData&		GetData() const		{ return fData; }
	COnDiskData&			GetData()			{ fDirty = true; return fData; }
	
	void					GetData(uint32 inIndex, string& outKey, uint32& outValue, uint32& outP) const;
	
	uint32					GetP0() const		{ return fData.p0; }
	uint32					GetParent() const	{ return fData.pp; }
	uint32					GetN() const		{ return fData.n; }
	uint32					GetP(uint32 inIndex) const;
	
	uint32					GetIndexForP(uint32 inP) const;
	
	void					SetValue(uint32 inIndex, uint32 inValue);

	uint32					FreeSpace() const;
	bool					CanStore(const string& inKey) const;

	unsigned char*			GetKey(uint32 inIndex);
	const unsigned char*	GetKey(uint32 inIndex) const;

	void					Insert(int32 inIndex, const string& inKey, uint32 inValue, uint32 inP);
	void					Delete(int32 inIndex);
	void					Copy(CIndexPage& inFromPage, int32 inFromIndex, int32 inToIndex, int32 inCount);

	void					Read();
	void					Write();

	void					SetParent(uint32 inParent);

  private:

	void					SwapBytesHtoN();
	void					SwapBytesNtoH();

	COnDiskData				fData;
	HStreamBase*			fFile;
	int64					fBaseOffset;
	uint32					fOffset;
	bool					fDirty;
};

CIndexPage::CIndexPage()
	: fFile(nil)
	, fBaseOffset(0)
	, fDirty(false)
{
}

CIndexPage::CIndexPage(HStreamBase& inFile, int64 inBaseOffset)
	: fFile(&inFile)
	, fBaseOffset(inBaseOffset)
	, fDirty(false)
{
	Allocate();
}

CIndexPage::CIndexPage(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset)
	: fFile(&inFile)
	, fBaseOffset(inBaseOffset)
	, fOffset(inOffset)
	, fDirty(false)
{
	Read();
}

CIndexPage::~CIndexPage()
{
	if (fDirty)
		Write();
}

void CIndexPage::Load(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset)
{
	assert(fDirty == false);
	
	if (fFile != &inFile or fBaseOffset != inBaseOffset or fOffset != inOffset)
	{
		fFile = &inFile;
		fBaseOffset = inBaseOffset;
		fOffset = inOffset;
		
		Read();
	}
}

void CIndexPage::Allocate()
{
	if (fDirty)
		Write();
	
	memset(&fData, 0, kPageSize);

	fFile->Seek(0, SEEK_END);
	fOffset = static_cast<uint32>(fFile->Tell() - fBaseOffset);
	if (fOffset == 0) // avoid returning a zero address!!!
	{
		char c = 0xff;
		fFile->Write(&c, 1);
		fOffset = static_cast<uint32>(fFile->Tell() - fBaseOffset);
		assert(fOffset == 1);
	}

	// allocate on disk	
	Write();
}

unsigned char* CIndexPage::GetKey(uint32 inIndex)
{
	if (inIndex > fData.n)
		THROW(("Key index out of range"));
	
	unsigned char* k = fData.keys;
	while (inIndex-- > 0)
		k += k[0] + 1;
	
	return k;
}

const unsigned char* CIndexPage::GetKey(uint32 inIndex) const
{
	if (inIndex > fData.n)
		THROW(("Key index out of range"));
	
	const unsigned char* k = fData.keys;
	while (inIndex-- > 0)
		k += k[0] + 1;
	
	return k;
}

void CIndexPage::GetData(uint32 inIndex, string& outKey, uint32& outValue, uint32& outP) const
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	const unsigned char* k = GetKey(inIndex);
	outKey.assign(reinterpret_cast<const char*>(k) + 1, k[0]);
	outValue = fData.e[-static_cast<int32>(inIndex)].value;
	outP = fData.e[-static_cast<int32>(inIndex)].p;
}

uint32 CIndexPage::GetP(uint32 inIndex) const
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	return fData.e[-static_cast<int32>(inIndex)].p;
}

uint32 CIndexPage::GetIndexForP(uint32 inP) const
{
	uint32 ix = 0;

	while (ix < fData.n)
	{
		if (fData.e[-static_cast<int32>(ix)].p == inP)
			break;
		++ix;
	}

	return ix;
}

void CIndexPage::SetValue(uint32 inIndex, uint32 inValue)
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	fData.e[-static_cast<int32>(inIndex)].value = inValue;
	fDirty = true;
}

void CIndexPage::SetParent(uint32 inParent)
{
	if (fData.pp != inParent)
	{
		fData.pp = inParent;
		fDirty = true;
	}
	
	if (fData.p0 != 0)
	{
		CIndexPage p0(*fFile, fBaseOffset, fData.p0);
		p0.SetParent(fOffset);
	}
	
	for (int32 i = 0; i < fData.n; ++i)
	{
		if (fData.e[-i].p != 0)
		{
			CIndexPage p(*fFile, fBaseOffset, fData.e[-i].p);
			p.SetParent(fOffset);
		}
	}
}

inline
uint32 CIndexPage::FreeSpace() const
{
	uint32 result = static_cast<uint32>(kKeySpace - fData.n * kEntrySize);

	const unsigned char* k = GetKey(fData.n);
	result -= k - fData.keys;
	
	return result;
}

inline
bool CIndexPage::CanStore(const string& inKey) const
{
	return FreeSpace() >= inKey.length() + kEntrySize + 1;
}

void CIndexPage::Insert(int32 inIndex, const string& inKey, uint32 inValue, uint32 inP)
{
	fDirty = true;

	assert(inIndex >= 0);
	assert(inIndex <= fData.n);

#if P_DEBUG
	uint32 free = FreeSpace();
	assert(inKey.length() + 1 + kEntrySize <= free);
#endif
	
	unsigned char* k = GetKey(inIndex);
	
	if (inIndex < fData.n)
	{
		unsigned char* lk = GetKey(fData.n - 1);
		
		int32 m = (lk + *lk + 1) - k;
		assert(m > 0);
		
		if (m > 0)
			memmove(k + inKey.length() + 1, k, static_cast<uint32>(m));
	
		for (int32 i = -static_cast<int32>(fData.n); i < -static_cast<int32>(inIndex); ++i)
			fData.e[i] = fData.e[i + 1];
	}

	k[0] = static_cast<uint8>(inKey.length());
	inKey.copy(reinterpret_cast<char*>(k) + 1, inKey.length());
	
	fData.e[-static_cast<int32>(inIndex)].value = inValue;
	fData.e[-static_cast<int32>(inIndex)].p = inP;
	++fData.n;
}

void CIndexPage::Delete(int32 inIndex)
{
	fDirty = true;
	
	assert(inIndex >= 0);
	assert(inIndex < fData.n);
	
	unsigned char* k = GetKey(inIndex);
	
	if (inIndex < fData.n)
	{
		unsigned char* fk = GetKey(inIndex + 1);
		unsigned char* lk = GetKey(fData.n - 1);
		
		lk += *lk + 1;

		int32 m = lk - fk;
		assert(m >= 0);
//		assert(m + inKey.length() + 1 <= free);
		
		if (m > 0)
			memmove(k, fk, static_cast<uint32>(m));
	
		for (int32 i = static_cast<int32>(inIndex); i < static_cast<int32>(fData.n) - 1; ++i)
			fData.e[-i] = fData.e[-(i + 1)];
	}
}

void CIndexPage::Copy(CIndexPage& inFromPage, int32 inFromIndex, int32 inToIndex, int32 inCount)
{
	fDirty = true;

	while (inCount-- > 0)
	{
		assert(inFromIndex < inFromPage.fData.n);
		assert(inToIndex <= fData.n);
		
		string key;
		uint32 value;
		uint32 p;
		
		inFromPage.GetData(inFromIndex, key, value, p);
		
		Insert(inToIndex, key, value, p);
		
		++inFromIndex;
		++inToIndex;
	}
}

void CIndexPage::Read()
{
	uint32 read = fFile->PRead(&fData, kPageSize, fBaseOffset + fOffset);
	assert(read == kPageSize);
	if (read != kPageSize)
		THROW(("IO Error, reading page for index"));
	
	fDirty = false;

#if P_LITTLEENDIAN
	SwapBytesNtoH();
#endif
}

void CIndexPage::Write()
{
#if P_LITTLEENDIAN
	SwapBytesHtoN();
#endif
	
	uint32 written = fFile->PWrite(&fData, kPageSize, fBaseOffset + fOffset);
	assert(written == kPageSize);
	if (written != kPageSize)
		THROW(("IO Error, writing page for index"));
	
	fDirty = false;

#if P_LITTLEENDIAN
	SwapBytesNtoH();
#endif
}

void CIndexPage::SwapBytesHtoN()
{
	for (int32 i = 0; i < fData.n; ++i)
	{
		fData.e[-i].p = net_swapper::swap(fData.e[-i].p);
		fData.e[-i].value = net_swapper::swap(fData.e[-i].value);
	}
	fData.p0 = net_swapper::swap(fData.p0);
	fData.pp = net_swapper::swap(fData.pp);
	fData.n = net_swapper::swap(fData.n);
}

void CIndexPage::SwapBytesNtoH()
{
	fData.p0 = net_swapper::swap(fData.p0);
	fData.pp = net_swapper::swap(fData.pp);
	fData.n = net_swapper::swap(fData.n);
	for (int32 i = 0; i < fData.n; ++i)
	{
		fData.e[-i].p = net_swapper::swap(fData.e[-i].p);
		fData.e[-i].value = net_swapper::swap(fData.e[-i].value);
	}
}

// ---------------------------------------------------------------------------

struct CIndexImp
{
  public:
					CIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot);
	virtual 		~CIndexImp();

	HStreamBase&	GetFile() const					{ return fFile; }
	int64			GetBaseOffset() const			{ return fBaseOffset; }
	uint32			GetRoot() const					{ return fRoot; }
	uint32			GetKind() const					{ return fKind; }

	CIndex::iterator	Begin();		// Begin is defined as the first entry on the left most leaf page
	CIndex::iterator	End();			// End is defined as N on the right most leaf page
	CIndex::iterator	LowerBound(const string& inKey);

	bool			GetValue(const string& inKey, uint32& outValue) const;
	void			GetValuesForPattern(const string& inKey, vector<uint32>& outValues) const;

	uint32			GetCount(uint32 inPage) const;
	void			Visit(uint32 inPage, CIndex::VisitorBase& inVisitor);

	void			CreateFromIterator(CIteratorBase& inIter);

 	virtual int		Compare(const char* inA, uint32 inLengthA,
						const char* inB, uint32 inLengthB) const;

	int				Compare(const string& inA, const string& inB) const
						{ return Compare(inA.c_str(), inA.length(), inB.c_str(), inB.length()); }

#if P_DEBUG
	void			Test(CIndex& inIndex);
	void			Dump(uint32 inPage, uint32 inLevel);
#endif

  private:

	void			LowerBoundImp(const string& inKey, uint32 inPage, uint32& outPage, uint32& outIndex);

	HStreamBase&	fFile;
	int64			fBaseOffset;
	uint32			fRoot;
	uint32			fKind;
};

CIndexImp::CIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: fFile(inFile)
	, fBaseOffset(inOffset)
	, fRoot(inRoot)
	, fKind(inKind)
{
}

CIndexImp::~CIndexImp()
{
}

int CIndexImp::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	return CompareKeyString(inA, inLengthA, inB, inLengthB);
}

CIndex::iterator CIndexImp::Begin()
{
	uint32 page = fRoot;
	CIndex::iterator result;
	
	while (page != 0)
	{
		CIndexPage p(fFile, fBaseOffset, page);
		
		result = CIndex::iterator(fFile, fBaseOffset, page, 0);
		
		page = p.GetP0();
	}
	
	return result;
}

CIndex::iterator CIndexImp::End()
{
	CIndexPage p(fFile, fBaseOffset, fRoot);
	return CIndex::iterator(fFile, fBaseOffset, fRoot, p.GetN());
}

void CIndexImp::LowerBoundImp(const string& inKey, uint32 inPage, uint32& outPage, uint32& outIndex)
{
	const CIndexPage p(fFile, fBaseOffset, inPage);
	const COnDiskData& data = p.GetData();
	bool found = false;
	
	int32 L = 0;
	int32 R = static_cast<int32>(data.n) - 1;
	while (L <= R)
	{
		int32 i = (L + R) / 2;
		const unsigned char* key = p.GetKey(i);
		
		int d = Compare(inKey.c_str(), inKey.length(),
			reinterpret_cast<const char*>(key + 1), key[0]);
		
		if (d == 0)
		{
			found = true;
			outPage = inPage;
			outIndex = i;
			break;
		}
		else if (d < 0)
			R = i - 1;
		else
			L = i + 1;
	}
	
	if (not found)
	{
		// init the outPage and inIndex to End() if needed
		
		if (inPage == fRoot)
		{
			outPage = inPage;
			outIndex = data.n;
		}
		
		// suppose we're looking for a key that's larger than the last entry.
		// In that case we need to return the same value as End(). This means
		// we have to be careful, the outPage and outIndex have been inited to
		// the values for End() so we override them only if the key is actually
		// smaller than our largest value, i.e. R < N;
		
		if (R < static_cast<int32>(data.n))
		{
			outPage = inPage;
			outIndex = static_cast<uint32>(R + 1);
		}
		
		if (R >= 0)
			LowerBoundImp(inKey, data.e[-R].p, outPage, outIndex);
		else
			LowerBoundImp(inKey, data.p0, outPage, outIndex);
	}
}

CIndex::iterator CIndexImp::LowerBound(const string& inKey)
{
	uint32 page = 0, index = 0;

	LowerBoundImp(inKey, fRoot, page, index);
	
	return CIndex::iterator(fFile, fBaseOffset, page, index);
}

bool CIndexImp::GetValue(const string& inKey, uint32& outValue) const
{
	bool found = false;

	uint32 page = fRoot;
	
	do
	{
		const CIndexPage p(fFile, fBaseOffset, page);
		const COnDiskData& data = p.GetData();
		
		int32 L = 0;
		int32 R = static_cast<int32>(data.n) - 1;
		while (L <= R)
		{
			int32 i = (L + R) / 2;
			const unsigned char* key = p.GetKey(i);
			
			int d = Compare(inKey.c_str(), inKey.length(),
				reinterpret_cast<const char*>(key + 1), key[0]);

			if (d == 0)
			{
				found = true;
				outValue = data.e[-i].value;
				break;
			}
			else if (d < 0)
				R = i - 1;
			else
				L = i + 1;
		}
		
		if (R >= 0)
			page = data.e[-R].p;
		else
			page = data.p0;
	}
	while (not found and page != 0);

	return found;
}

void CIndexImp::GetValuesForPattern(const string& inKey, vector<uint32>& outValues) const
{
	uint32 page = fRoot;
	
	pair<uint32,uint32> v(page, 0);
	stack<pair<uint32,uint32> > pStack;

	uint32 c, val;
	string key;
	
	while (page != 0)
	{
		const CIndexPage p(fFile, fBaseOffset, page);

		int32 L = 0;
		int32 R = static_cast<int32>(p.GetN()) - 1;
		while (L <= R)
		{
			int32 i = (L + R) / 2;

			p.GetData(i, key, val, c);
		
			MatchResult m = Match(inKey.c_str(), key.c_str());

			if (m == eNoMatchAndLess)
				L = i + 1;
			else
				R = i - 1;
		}
		
		v.first = page;
		
		if (L > 0)
			v.second = L - 1;
		else
			v.second = 0;

		pStack.push(v);
		
		if (R < 0)
			page = p.GetP0();
		else
			page = p.GetData().e[-R].p;
	}

	bool done = false;
	
	while (not done and not pStack.empty())
	{
		v = pStack.top();
		const CIndexPage p(fFile, fBaseOffset, v.first);
		
		if (v.second < p.GetN())
		{
			p.GetData(v.second, key, val, c);
			++pStack.top().second;
			
			MatchResult m = Match(inKey.c_str(), key.c_str());

			if (m == eMatch)
				outValues.push_back(val);
			else if (m == eNoMatchAndGreater)
				done = true;
			
			if (not (m == eNoMatchAndLess and v.second == p.GetN() - 1))
			{
				while (c != 0)
				{
					v.first = c;
					v.second = 0;
				
					pStack.push(v);
					
					const CIndexPage cp(fFile, fBaseOffset, c);
					c = cp.GetP0();
				}
			}
		}
		else
			pStack.pop();
	}
}

uint32 CIndexImp::GetCount(uint32 inPage) const
{
	uint32 result = 0;
	
	if (inPage != 0)
	{
		const CIndexPage p(fFile, fBaseOffset, inPage);
		result = p.GetData().n;
		
		result += GetCount(p.GetP0());
		
		for (uint32 i = 0; i < p.GetN(); ++i)
			result += GetCount(p.GetP(i));
	}

	return result;		
}

void CIndexImp::Visit(uint32 inPage, CIndex::VisitorBase& inVisitor)
{
	if (inPage != 0)
	{
		CIndexPage p(fFile, fBaseOffset, inPage);
		uint32 n = p.GetN();
		
		if (p.GetP0())
			Visit(p.GetP0(), inVisitor);
		
		for (uint32 i = 0; i < n; ++i)
		{
			string k;
			uint32 ov, nv, c;
			
			p.GetData(i, k, ov, c);
			nv = ov;
			
			inVisitor.Visit(k, nv);
			
			if (ov != nv)
				p.SetValue(i, nv);
			
			if (p.GetP(i))
				Visit(p.GetP(i), inVisitor);
		}
	}
}

struct CTempValue
{
	string		key;
	uint32		value;
	uint32		pageLeft;
	uint32		pageRight;
};

typedef list<CTempValue>	CTempValueList;

void CIndexImp::CreateFromIterator(CIteratorBase& inData)
{
	// first pass, collect the data from the iterator building the leaf pages
	
	string k;
	uint32 v;
	
	CIndexPage p(fFile, fBaseOffset);
	CTempValueList up;

	fRoot = p.GetOffset();

	while (inData.Next(k, v))
	{
		if (not p.CanStore(k))
		{
			CTempValue t;
			
			t.key = k;
			t.value = v;
			t.pageLeft = p.GetOffset();
			
			p.Allocate();
			
			t.pageRight = p.GetOffset();
			
			up.push_back(t);
		}
		else
			p.Insert(p.GetN(), k, v, 0);
	}
	
	// next passes, iterate over the up list to create new next-higher level pages
	while (up.size())
	{
		// it can happen that the last item in up has a pageRight to an empty
		// page since the item that triggered its creation was the last in line.
		// in that case, p is still this empty page and up.back().pageRight is
		// the pointer to it. Reset the pointer and reuse the page.

		if (p.GetN() == 0)
		{
			assert(up.back().pageRight = p.GetOffset());
			up.back().pageRight = 0;
		}
		else
			p.Allocate();

		fRoot = p.GetOffset();
		
		p.GetData().p0 = up.front().pageLeft;
		
		CTempValueList nextUp;
		
		for (CTempValueList::iterator i = up.begin(); i != up.end(); ++i)
		{
			if (not p.CanStore(i->key))
			{
				CTempValue t = {};
				
				t.key = i->key;
				t.value = i->value;
				t.pageLeft = p.GetOffset();
				
				p.Allocate();
				p.GetData().p0 = i->pageRight;
				
				t.pageRight = p.GetOffset();
				
				nextUp.push_back(t);
			}
			else
				p.Insert(p.GetN(), i->key, i->value, i->pageRight);
		}
		
		up = nextUp;
	}
	
	// yeah well, unfortunately we have to traverse the entire tree
	// again to store parent pointers...
	
	assert(p.GetOffset() == fRoot);
	p.SetParent(0);
}

#if P_DEBUG
void CIndexImp::Test(CIndex& inIndex)
{
	vector<string> keys;
	
	for (CIndex::iterator i = Begin(); i != End(); ++i)
		keys.push_back(i->first);
	
	for (vector<string>::iterator k = keys.begin(); k != keys.end(); ++k)
	{
		CIndex::iterator i = inIndex.find(*k);
		
		assert(i != inIndex.end());
		
		uint32 n = 0;
		while (i != inIndex.end())
			++i, ++n;
		
		assert(n == (keys.end() - k));

		i = inIndex.find(*k);
		
		assert(i != inIndex.end());
		
		n = 0;
		while (i != inIndex.begin())
			--i, ++n;
		
		assert(n == (k - keys.begin()));
	}
}

void CIndexImp::Dump(uint32 inPage, uint32 inLevel)
{
	const CIndexPage p(fFile, fBaseOffset, inPage);

	for (uint32 j = 0; j < inLevel; ++j)
		cout << "\t";
	cout << "dumping page " << inPage << ", parent = " << p.GetParent() << endl;

	for (uint32 i = 0; i < p.GetN(); ++i)
	{
		for (uint32 j = 0; j < inLevel; ++j)
			cout << "\t";
		
		string k;
		uint32 v;
		uint32 c;
		
		p.GetData(i, k, v, c);
		
		cout << "ix: " << i << ", p: " << c << ", key: " << k << endl;
	}

	if (p.GetP0())
		Dump(p.GetP0(), inLevel + 1);
	
	for (uint32 i = 0; i < p.GetN(); ++i)
	{
		string k;
		uint32 v;
		uint32 c;
		
		p.GetData(i, k, v, c);
		
		if (c != 0)
			Dump(c, inLevel + 1);
	}

	cout << endl;
}
#endif

// ---------------------------------------------------------------------------

class CNumberIndexImp : public CIndexImp
{
  public:
					CNumberIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
						: CIndexImp(inKind, inFile, inOffset, inRoot) {}

	virtual int		Compare(const char* inA, uint32 inLengthA,
						const char* inB, uint32 inLengthB) const;
};

int CNumberIndexImp::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	return CompareKeyNumber(inA, inLengthA, inB, inLengthB);
}

// ---------------------------------------------------------------------------

// normal constructor for an exisiting index on disk
CIndex::CIndex(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
{
	switch (inKind)
	{
		case kTextIndex:
		case kDateIndex:
		case kValueIndex:
		case kWeightedIndex:
			fImpl = new CIndexImp(inKind, inFile, inOffset, inRoot);
			break;
		
		case kNumberIndex:
			fImpl = new CNumberIndexImp(inKind, inFile, inOffset, inRoot);
			break;
		
		default:
			THROW(("Invalid index kind: %4.4s", &inKind));
			break;
	}
}

CIndex::~CIndex()
{
	delete fImpl;
}

// static factories to create new indices on disk
CIndex* CIndex::Create(uint32 inIndexKind, HStreamBase& inFile)
{
	int64 offset = inFile.Seek(0, SEEK_END);
	return new CIndex(inIndexKind, inFile, offset, 0);
}

// copy creates a compacted copy
CIndex* CIndex::CreateFromIterator(uint32 inIndexKind, CIteratorBase& inData, HStreamBase& inFile)
{
	auto_ptr<CIndex> result(Create(inIndexKind, inFile));
	result->fImpl->CreateFromIterator(inData);
	
	inFile.Seek(0, SEEK_END);
	
//#if P_DEBUG
//	result->fImpl->Test(*result);
//#endif
	
	return result.release();
}

// access data
bool CIndex::GetValue(const string& inKey, uint32& outValue) const
{
	return fImpl->GetValue(inKey, outValue);
}

void CIndex::GetValuesForPattern(const string& inKey, std::vector<uint32>& outValues)
{
	fImpl->GetValuesForPattern(inKey, outValues);
}

void CIndex::GetValuesForRange(const char* inLowerBound,
	const char* inUpperBound, std::vector<uint32>& outValues)
{
	CIndex::iterator b, e, i;
	
	if (inLowerBound)
		b = lower_bound(inLowerBound);
	else
		b = begin();
	
	if (inUpperBound)
		e = upper_bound(inUpperBound);
	else
		e = end();
	
	for (i = b; i != e; ++i)
		outValues.push_back(i->second);
}

void CIndex::GetValuesForOperator(const string& inKey, CQueryOperator inOperator,
	vector<uint32>& outValues)
{
	CIndex::iterator b, e, i;
	
	switch (inOperator)
	{
		case kOpEquals:
			b = lower_bound(inKey);
			e = upper_bound(inKey);
			break;
		
		case kOpLessThan:
			b = begin();
			e = lower_bound(inKey);
			break;
		
		case kOpLessEqual:
			b = begin();
			e = upper_bound(inKey);
			break;
		
		case kOpGreaterEqual:
			b = lower_bound(inKey);
			e = end();
			break;
		
		case kOpGreaterThan:
			b = upper_bound(inKey);
			e = end();
			break;
		
		default:
			break;
	}

	for (i = b; i != e; ++i)
		outValues.push_back(i->second);
}

uint32 CIndex::GetRoot() const
{
	return fImpl->GetRoot();
}

uint32 CIndex::GetCount() const
{
	return fImpl->GetCount(fImpl->GetRoot());
}

uint32 CIndex::GetKind() const
{
	return fImpl->GetKind();
}

void CIndex::Visit(VisitorBase& inVisitor)
{
	fImpl->Visit(fImpl->GetRoot(), inVisitor);
}

CIndex::iterator CIndex::begin()
{
	return fImpl->Begin();
}

CIndex::iterator CIndex::end()
{
	return fImpl->End();
}

CIndex::iterator CIndex::find(const string& inKey)
{
	iterator result = fImpl->LowerBound(inKey);
	if (result->first != inKey)
		result = end();
	return result;
}

CIndex::iterator CIndex::lower_bound(const string& inKey)
{
	return fImpl->LowerBound(inKey);
}

CIndex::iterator CIndex::upper_bound(const string& inKey)
{
	iterator result = lower_bound(inKey);
	iterator _end(end());
	
	while (result != _end and fImpl->Compare(result->first, inKey) <= 0)
		++result;
	
	return result;
}

#if P_DEBUG
void CIndex::Dump() const
{
	fImpl->Dump(fImpl->GetRoot(), 0);
}
#endif

// ---------------------------------------------------------------------------
//
// iterator	(boost rules!)
//

CIndex::iterator::iterator()
	: fFile(nil)
	, fBaseOffset(0)
	, fPage(0)
	, fPageIndex(0)
{
}

CIndex::iterator::iterator(HStreamBase& inFile, int64 inOffset,
		uint32 inPage, uint32 inPageIndex)
	: fFile(&inFile)
	, fBaseOffset(inOffset)
	, fPage(inPage)
	, fPageIndex(inPageIndex)
{
	CIndexPage p(*fFile, fBaseOffset, fPage);
	uint32 c;

	if (fPageIndex < p.GetN())
		p.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
}

CIndex::iterator::iterator(const iterator& inOther)
	: fFile(inOther.fFile)
	, fBaseOffset(inOther.fBaseOffset)
	, fPage(inOther.fPage)
	, fPageIndex(inOther.fPageIndex)
	, fCurrent(inOther.fCurrent)
{
}

CIndex::iterator& CIndex::iterator::operator=(const iterator& inOther)
{
	if (this != &inOther)
	{
		fFile = inOther.fFile;
		fBaseOffset = inOther.fBaseOffset;
		fPage = inOther.fPage;
		fPageIndex = inOther.fPageIndex;
		fCurrent = inOther.fCurrent;
	}

	return *this;
}

void CIndex::iterator::increment()
{
	CIndexPage p(*fFile, fBaseOffset, fPage);
	bool valid = true;

	if (p.GetP(fPageIndex)) // current entry has a p, decent
	{
		fPage = p.GetP(fPageIndex);
		fPageIndex = 0;

		p.Load(*fFile, fBaseOffset, fPage);
		
		while (p.GetP0())
		{
			fPage = p.GetP0();
			p.Load(*fFile, fBaseOffset, fPage);
		}
	}
	else
	{
		++fPageIndex;

		while (fPageIndex >= p.GetN() and p.GetParent())	// do we have a parent?
		{
			p.Load(*fFile, fBaseOffset, p.GetParent());

			if (p.GetP0() == fPage)
				fPageIndex = 0;
			else
				fPageIndex = p.GetIndexForP(fPage) + 1;

			fPage = p.GetOffset();
		}
		
		valid = (fPageIndex < p.GetN());
	}
	
	if (valid)
	{
		uint32 c;
		p.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
	}
}

void CIndex::iterator::decrement()
{
	CIndexPage p(*fFile, fBaseOffset, fPage);

	uint32 dp = 0;

	if (fPageIndex == 0)		// beginning of page
	{
		if (p.GetP0())			// see if we need to decent down p0 first
			dp = p.GetP0();
		else
		{
			uint32 np = fPage;		// tricky, see if we can find a page upwards
			uint32 nx = fPageIndex;	// where we hit an index > 0. If not we're at
									// the far left and thus Begin()
			
			while (p.GetParent())
			{
				p.Load(*fFile, fBaseOffset, p.GetParent());
				
				if (np == p.GetP0())
				{
					np = p.GetOffset();
					nx = 0;
				}
				else
				{
					fPageIndex = p.GetIndexForP(np);
					fPage = p.GetOffset();
					break;
				}
			}
		}
	}
	else
	{
		--fPageIndex;
		dp = p.GetP(fPageIndex);
	}
	
	while (dp)
	{
		p.Load(*fFile, fBaseOffset, dp);
		fPage = dp;
		fPageIndex = p.GetN() - 1;
		dp = p.GetP(fPageIndex);
	}
	
	p.Load(*fFile, fBaseOffset, fPage);
	uint32 c;
	p.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
}

bool CIndex::iterator::equal(const iterator& inOther) const
{
	assert(fFile == inOther.fFile);
	assert(fBaseOffset == inOther.fBaseOffset);

	return
		fFile == inOther.fFile and fBaseOffset == inOther.fBaseOffset and
		fPage == inOther.fPage and fPageIndex == inOther.fPageIndex;
}

const pair<string,uint32>& CIndex::iterator::dereference() const
{
	return fCurrent;
}

