/*	$Id: CIndexPage.cpp,v 1.74 2005/09/15 10:12:08 maarten Exp $
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

#include "CIndexPage.h"
#include "HStream.h"
#include "CUtils.h"
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
	kKeySpace = kPageSize - kEntrySize - 2 * sizeof(uint32),
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
	uint32					GetN() const		{ return fData.n; }
	uint32					GetP(uint32 inIndex) const;
	
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

void CIndexPage::SetValue(uint32 inIndex, uint32 inValue)
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	fData.e[-static_cast<int32>(inIndex)].value = inValue;
	fDirty = true;
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
		THROW(("IO Error, reading page for index"));
	
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
	fData.n = net_swapper::swap(fData.n);
}

void CIndexPage::SwapBytesNtoH()
{
	fData.p0 = net_swapper::swap(fData.p0);
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

	bool			GetValue(const string& inKey, uint32& outValue, CIndex::Path* outPath) const;
	void			GetValuesForPattern(const string& inKey, vector<uint32>& outValues) const;
	void			SetValue(const std::string& inKey, uint32 inValue);
	void			SetValue(uint32 inPage, const std::string& inKey, uint32 inValue,
						std::string& outKey, uint32& outValue, uint32& outP,
						bool& outHeightIncrease);

	uint32			GetCount(uint32 inPage) const;
	void			Visit(uint32 inPage, CIndex::VisitorBase& inVisitor);

	void			CreateFromIterator(CIteratorBase& inIter);

 	virtual int		Compare(const char* inA, uint32 inLengthA,
						const char* inB, uint32 inLengthB) const;

	int				Compare(const string& inA, const string& inB) const
						{ return Compare(inA.c_str(), inA.length(), inB.c_str(), inB.length()); }

#if P_DEBUG
	void			Dump(uint32 inPage, uint32 inLevel);
#endif

  private:

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

bool CIndexImp::GetValue(const string& inKey, uint32& outValue, CIndex::Path* outPath) const
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
				if (outPath != nil)
					outPath->push(make_pair(page, static_cast<uint32>(i)));
				outValue = data.e[-i].value;
				break;
			}
			else if (d < 0)
				R = i - 1;
			else
				L = i + 1;
		}
		
		if (outPath != nil)
		{
			if (found)
				outPath->push(make_pair(page, static_cast<uint32>((L + R) / 2)));
			else
				outPath->push(make_pair(page, static_cast<uint32>(R + 1)));
		}
		
		if (R >= 0)
			page = data.e[-R].p;
		else
			page = data.p0;
	}
	while (not found and page != 0);
	
	// special case, avoid returning a path that points after the last element
	// end() in our case means an empty path...
	if (not found and outPath != nil)
	{
		while (not outPath->empty())
		{
			const CIndexPage p(fFile, fBaseOffset, outPath->top().first);
			
			if (outPath->top().second >= p.GetN())
				outPath->pop();
			else
				break;
		}
	}
	
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

void CIndexImp::SetValue(const std::string& inKey, uint32 inValue)
{
	string key;
	uint32 value;
	uint32 p;
	bool h;
	
	assert(inKey.length() < kMaxKeySize);

	SetValue(fRoot, inKey, inValue, key, value, p, h);
	
	if (h)
	{
		CIndexPage page(fFile, fBaseOffset);
		COnDiskData& data = page.GetData();
		
		data.p0 = fRoot;
		page.Insert(0, key, value, p);
		
		fRoot = page.GetOffset();
	}
}

void CIndexImp::SetValue(uint32 inPage, const std::string& inKey, uint32 inValue,
	std::string& outKey, uint32& outValue, uint32& outP, bool& outHeightIncrease)
{
	// shortcut
	if (inPage == 0)
	{
		outHeightIncrease = true;
		outKey = inKey;
		outValue = inValue;
		outP = 0;
		return;
	}
	
	CIndexPage p(fFile, fBaseOffset, inPage);
	COnDiskData& data = p.GetData();
	
	int32 L = 0;
	int32 R = static_cast<int32>(data.n) - 1;
	while (L <= R)
	{
		int32 i = (L + R) / 2;
		const unsigned char* key = p.GetKey(i);
		
		int l = inKey.length();
		if (l > key[0])
			l = key[0];

		int d = Compare(inKey.c_str(), inKey.length(),
			reinterpret_cast<const char*>(key + 1), key[0]);

		if (d == 0)
		{
			data.e[-i].value = inValue;
			outHeightIncrease = false;

			return;		// ooh, I hate that...
		}

		if (d < 0)
			R = i - 1;
		else
			L = i + 1;
	}
	
	uint32 child;
	if (R >= 0)
		child = data.e[-R].p;
	else
		child = data.p0;
	
	string uKey;
	uint32 uValue, uP;

	if (child)
		SetValue(child, inKey, inValue, uKey, uValue, uP, outHeightIncrease);
	else
	{
		outHeightIncrease = true;
		uKey = inKey;
		uValue = inValue;
		uP = 0;
	}
	
	if (outHeightIncrease)
	{
		if (p.CanStore(uKey))
		{
			outHeightIncrease = false;
			p.Insert(R + 1, uKey, uValue, uP);
		}
		else	// split
		{
			CIndexPage q(fFile, fBaseOffset);
			
			int32 h = data.n / 2;
			
			p.GetData(h, outKey, outValue, outP);
			
			q.GetData().p0 = outP;
			q.Copy(p, h + 1, 0, data.n - h - 1);
			data.n = h;
			
			if (R >= h)
				q.Insert(R - data.n, uKey, uValue, uP);
			else
				p.Insert(R + 1, uKey, uValue, uP);
			
			outP = q.GetOffset();
		}
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
}

#if P_DEBUG
void CIndexImp::Dump(uint32 inPage, uint32 inLevel)
{
	const CIndexPage p(fFile, fBaseOffset, inPage);

//	if (p.GetP0())
//		Dump(p.GetP0(), inLevel + 1);
	
	for (uint32 i = 0; i < p.GetN(); ++i)
	{
		for (uint32 j = 0; j < inLevel; ++j)
			cout << "\t";
		
		string k;
		uint32 v;
		uint32 c;
		
		p.GetData(i, k, v, c);
		
		cout << k << endl;
		
//		if (c != 0)
//			Dump(c, inLevel + 1);
	}

	if (p.GetP0())
		Dump(p.GetP0(), inLevel + 1);
	
	for (uint32 i = 0; i < p.GetN(); ++i)
	{
//		for (uint32 j = 0; j < inLevel; ++j)
//			cout << "\t";
		
		string k;
		uint32 v;
		uint32 c;
		
		p.GetData(i, k, v, c);
		
//		cout << k << endl;
		
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
CIndex* CIndex::Create(uint inIndexKind, HStreamBase& inFile)
{
	int64 offset = inFile.Seek(0, SEEK_END);

	CIndexPage page(inFile, offset);

	inFile.Seek(0, SEEK_END);
	
	return new CIndex(inIndexKind, inFile, offset, page.GetOffset());
}

// copy creates a compacted copy
CIndex* CIndex::CreateFromIterator(uint32 inIndexKind, CIteratorBase& inData, HStreamBase& inFile)
{
	auto_ptr<CIndex> result(Create(inIndexKind, inFile));
	result->fImpl->CreateFromIterator(inData);
	
	inFile.Seek(0, SEEK_END);
	
	return result.release();
}

// access data
bool CIndex::GetValue(const string& inKey, uint32& outValue) const
{
	return fImpl->GetValue(inKey, outValue, nil);
}

void CIndex::SetValue(const string& inKey, uint32 inValue)
{
	if (inKey.length() == 0)
		THROW(("Cannot insert empty strings into an index"));
	
	fImpl->SetValue(inKey, inValue);
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
	return iterator(fImpl->GetFile(), fImpl->GetBaseOffset(), fImpl->GetRoot());
}

CIndex::iterator CIndex::end()
{
	return iterator(fImpl->GetFile(), fImpl->GetBaseOffset(), fImpl->GetRoot(), Path());
}

CIndex::iterator CIndex::find(const string& inKey)
{
	Path path;
	iterator result;
	uint32 v;
	
	if (fImpl->GetValue(inKey, v, &path))
		result = iterator(fImpl->GetFile(), fImpl->GetBaseOffset(), fImpl->GetRoot(), path);
	else
		result = iterator(fImpl->GetFile(), fImpl->GetBaseOffset(), fImpl->GetRoot(), Path());
	
	return result;
}

CIndex::iterator CIndex::lower_bound(const string& inKey)
{
	Path path;
	uint32 v;
	
	(void)fImpl->GetValue(inKey, v, &path);
	return iterator(fImpl->GetFile(), fImpl->GetBaseOffset(), fImpl->GetRoot(), path);
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
	, fRoot(0)
{
}

CIndex::iterator::iterator(HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: fFile(&inFile)
	, fBaseOffset(inOffset)
	, fRoot(inRoot)
{
	uint32 page = inRoot;	

	while (page != 0)
	{
		fStack.push(make_pair(page, 0));

		const CIndexPage p(*fFile, fBaseOffset, page);
		page = p.GetP0();

		uint32 c;
		p.GetData(0, fCurrent.first, fCurrent.second, c);
	}
}

CIndex::iterator::iterator(HStreamBase& inFile, int64 inOffset, uint32 inRoot, const Path& inPath)
	: fFile(&inFile)
	, fBaseOffset(inOffset)
	, fRoot(inRoot)
	, fStack(inPath)
{
	if (not fStack.empty())
	{
		const CIndexPage p(*fFile, fBaseOffset, fStack.top().first);
		uint32 c;

		p.GetData(fStack.top().second, fCurrent.first, fCurrent.second, c);
	}
}

CIndex::iterator::iterator(const iterator& inOther)
	: fFile(inOther.fFile)
	, fBaseOffset(inOther.fBaseOffset)
	, fRoot(inOther.fRoot)
	, fStack(inOther.fStack)
	, fCurrent(inOther.fCurrent)
{
}

CIndex::iterator& CIndex::iterator::operator=(const iterator& inOther)
{
	if (this != &inOther)
	{
		fFile = inOther.fFile;
		fBaseOffset = inOther.fBaseOffset;
		fRoot = inOther.fRoot;
		fStack = inOther.fStack;
		fCurrent = inOther.fCurrent;
	}

	return *this;
}

void CIndex::iterator::increment()
{
	// four steps:
	// 1. the stack is empty, that's easy, don't do anything
	// 2. if the current item on the current page has a p, descent down this path
	//    to the lowest page to the first leaf item and stop here.
	// 3. if the item is not the last on this page, go to the item right of the current
	//    and stop here.
	// 4. Pop the stack once. The item on top is our current item if the page we
	//    just popped is the p0 of the current top page. Otherwise continue with step 3.
	
		
	if (not fStack.empty())								// 1.
	{
		pair<uint32,uint32> v = fStack.top();
		CIndexPage p(*fFile, fBaseOffset, v.first);
		
		uint32 c = p.GetP(fStack.top().second);
		
		if (c != 0)										// 2.
		{
			do
			{
				fStack.push(make_pair(c, 0));
				
				p.Load(*fFile, fBaseOffset, c);
				c = p.GetP0();
			}
			while (c != 0);
		}
		else
		{
			while (not fStack.empty())
			{
				++fStack.top().second;
				
				if (fStack.top().second < p.GetN())		// 3.
					break;
										
				c = fStack.top().first;					// 4.
				fStack.pop();
				
				if (not fStack.empty())
				{
					p.Load(*fFile, fBaseOffset, fStack.top().first);
					if (c == p.GetP0())
						break;
				}
			}
		}
		
		if (not fStack.empty())
		{
			p.Load(*fFile, fBaseOffset, fStack.top().first);
			p.GetData(fStack.top().second, fCurrent.first, fCurrent.second, c);
		}
	}
}

void CIndex::iterator::decrement()
{
	// four steps:
	// 1. the stack is empty, descent down the right most p's starting with root
	//    and stop here.
	// 2. if the item on top is the first on this page and the page has a p0, descent
	//    down the right most p's of the pages starting with p0 and stop;
	// 3. if the item on top is the first on this page but does not have a p0, pop the
	//    stack and stop if the page we're comming from is not the p0 of the new top.
	//    Otherwise, continue this step.
	// 4. go to the item left and if it has a p, descent down the right most p's of this p

	CIndexPage p;
	
	if (fStack.empty())										// 1.
	{
		uint32 page = fRoot;
		
		while (page != 0)
		{
			p.Load(*fFile, fBaseOffset, page);
			
			fStack.push(make_pair(page, p.GetN() - 1));
			page = p.GetP(p.GetN() - 1);
		}
	}
	else
	{
		CIndexPage p(*fFile, fBaseOffset, fStack.top().first);
		
		if (fStack.top().second == 0 and p.GetP0() != 0)	// 2.
		{
			uint32 page = p.GetP0();

			while (page != 0)
			{
				p.Load(*fFile, fBaseOffset, page);
				
				fStack.push(make_pair(page, p.GetN() - 1));
				page = p.GetP(p.GetN() - 1);
			}
		}
		else
		{
			if (fStack.top().second == 0)					// 3.
			{
				while (not fStack.empty() and fStack.top().second == 0)
				{
					uint32 c = fStack.top().first;
					
					fStack.pop();
					
					if (not fStack.empty())
					{
						p.Load(*fFile, fBaseOffset, fStack.top().first);
						if (c != p.GetP0())
							break;
					}
				}
			}
			else											// 4.
			{
				--fStack.top().second;
				
				uint32 page = p.GetP(fStack.top().second);
	
				while (page != 0)
				{
					p.Load(*fFile, fBaseOffset, page);
					
					fStack.push(make_pair(page, p.GetN() - 1));
					page = p.GetP(p.GetN() - 1);
				}
			}
		}
	}

	// update fCurrent
	if (not fStack.empty())
	{
		p.Load(*fFile, fBaseOffset, fStack.top().first);
		
		uint32 c;
		p.GetData(fStack.top().second, fCurrent.first, fCurrent.second, c);
	}
}

bool CIndex::iterator::equal(const iterator& inOther) const
{
	assert(fFile == inOther.fFile);
	assert(fBaseOffset == inOther.fBaseOffset);

	bool result;

	// end()
	if (fStack.empty() and inOther.fStack.empty())
		result = true;
	// one empty stack
	else if (fStack.empty() != inOther.fStack.empty())
		result = false;
	// the top of the stack should be the same
	else
		result = fStack.top().first == inOther.fStack.top().first and
			fStack.top().second == inOther.fStack.top().second;
	return result;
}

const pair<string,uint32>& CIndex::iterator::dereference() const
{
	return fCurrent;
}

