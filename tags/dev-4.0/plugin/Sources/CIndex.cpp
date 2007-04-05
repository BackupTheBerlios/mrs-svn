/*	$Id: CIndex.cpp 331 2007-02-12 07:44:10Z hekkel $
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

#include <iostream>
#include <map>
#include <list>
#include <cstring>

#include "CIndex.h"
#include "CIterator.h"
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
//	There are already 3 versions of MRS in the wild with different BTree layouts
//	We will try to support all of them.

const uint32
	kPageSize = 2048,
	kEntrySize = 2 * sizeof(uint32),
	kKeySpace = kPageSize - kEntrySize - 3 * sizeof(uint32),
	kMaxKeySize = (kKeySpace / 4 > 255 ? 255 : kKeySpace / 4);

//	The MRS 1.0 version of the BTree

struct COnDiskDataV0
{
	unsigned char	keys[kKeySpace + sizeof(uint32)];
	struct
	{
		uint32		value_;
		uint32		p_;
	} e[1];
	uint32			p0;
	int32			n;
	
	uint32			GetValue(int32 inIndex) const
					{
						return net_swapper::swap(e[-inIndex].value_);
					}
	void			SetValue(int32 inIndex, uint32 inValue)
					{
						e[-inIndex].value_ = net_swapper::swap(inValue);
					}
	uint32			GetP(int32 inIndex) const
					{
						return net_swapper::swap(e[-inIndex].p_);
					}
	void			SetP(int32 inIndex, uint32 inP)
					{
						e[-inIndex].p_ = net_swapper::swap(inP);
					}
	
	static uint32	PageNrToAddr(uint32 inPageNr)	{ return inPageNr; }
	static uint32	PageAddrToNr(uint32 inPageAddr)	{ return inPageAddr; }

	void			SwapBytes();
};

void COnDiskDataV0::SwapBytes()
{
	n = byte_swapper::swap(n);
	p0 = byte_swapper::swap(p0);
}

//	The MRS 2.0 version of the BTree, used for indices with a bits 
//	section size smaller than 4 Gb.

template<class BYTE_SWAPPER>
struct COnDiskDataV1
{
	typedef BYTE_SWAPPER	swapper;
	
	unsigned char	keys[kKeySpace];
	struct
	{
		uint32		value_;
		uint32		p_;
	} e[1];
	uint32			p0;
	uint32			pp;
	int32			n;
	
	uint32			GetValue(int32 inIndex) const
					{
						return swapper::swap(e[-inIndex].value_);
					}
	void			SetValue(int32 inIndex, uint32 inValue)
					{
						e[-inIndex].value_ = swapper::swap(inValue);
					}
	uint32			GetP(int32 inIndex) const
					{
						return swapper::swap(e[-inIndex].p_);
					}
	void			SetP(int32 inIndex, uint32 inP)
					{
						e[-inIndex].p_ = swapper::swap(inP);
					}
	
	static uint32	PageNrToAddr(uint32 inPageNr)	{ return inPageNr; }
	static uint32	PageAddrToNr(uint32 inPageAddr)	{ return inPageAddr; }

	void			SwapBytes();
};

template<class BYTE_SWAPPER>
void COnDiskDataV1<BYTE_SWAPPER>::SwapBytes()
{
	n = byte_swapper::swap(n);
	p0 = byte_swapper::swap(p0);
	pp = byte_swapper::swap(pp);
}

typedef COnDiskDataV1<byte_swapper>		COnDiskDataV1S;
typedef COnDiskDataV1<no_swapper>		COnDiskDataV1N;

//	The MRS 2.0 version of the BTree, used for indices with a bits 
//	section size larger than 4 Gb.

template<class BYTE_SWAPPER>
struct COnDiskDataV2
{
	typedef BYTE_SWAPPER	swapper;
	
	uint32			p0;
	uint32			pp;
	int32			n;
	unsigned char	keys[kKeySpace];
	union
	{
		struct {
			int64	value : 40;
			uint32	p	  : 24;
		};
		int64		d;
	}				e[1];

	int64			GetValue(int32 inIndex) const
					{
						return swapper::swap(e[-inIndex].d) >> 24;
					}
	void			SetValue(int32 inIndex, int64 inValue)
					{
						int64 ev = swapper::swap(e[-inIndex].d);
						
						ev &= 0x0FFFFFFULL;
						ev |= inValue << 24;
						
						e[-inIndex].d = swapper::swap(ev);
					}
	uint32			GetP(int32 inIndex) const
					{
						return swapper::swap(e[-inIndex].d) & 0x0FFFFFFULL;
					}
	void			SetP(int32 inIndex, uint32 inP)
					{
						int64 ev = swapper::swap(e[-inIndex].d);
						
						ev &= ~0x0FFFFFFULL;
						ev |= (inP & 0x0FFFFFFULL);
						
						e[-inIndex].d = swapper::swap(ev);
					}

	static int64	PageNrToAddr(uint32 inPageNr)	{ return static_cast<int64>(inPageNr - 1) * kPageSize; }
	static uint32	PageAddrToNr(int64 inPageAddr)	{ return static_cast<uint32>(inPageAddr / kPageSize) + 1; }

	void			SwapBytes();
};

template<class BYTE_SWAPPER>
void COnDiskDataV2<BYTE_SWAPPER>::SwapBytes()
{
	n = byte_swapper::swap(n);
	p0 = byte_swapper::swap(p0);
	pp = byte_swapper::swap(pp);
}

typedef COnDiskDataV2<byte_swapper>		COnDiskDataV2S;
typedef COnDiskDataV2<no_swapper>		COnDiskDataV2N;

template<typename DD>
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

	const DD&				GetData() const		{ return fData; }
	DD&						GetData()			{ fDirty = true; return fData; }
	
	void					GetData(uint32 inIndex, string& outKey, int64& outValue, uint32& outP) const;
	
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

	void					Insert(int32 inIndex, const string& inKey, int64 inValue, uint32 inP);
	void					Delete(int32 inIndex);
	void					Copy(CIndexPage& inFromPage, int32 inFromIndex, int32 inToIndex, int32 inCount);

	void					Read();
	void					Write();

	void					SetParent(uint32 inParent);

  private:
	DD						fData;
	HStreamBase*			fFile;
	int64					fBaseOffset;
	uint32					fOffset;
	bool					fDirty;
};

template<>
uint32 CIndexPage<COnDiskDataV0>::GetParent() const
{
	assert(false);
	THROW(("GetParent is not valid for this index version"));
	return 0;
}

template<typename DD>
CIndexPage<DD>::CIndexPage()
	: fFile(nil)
	, fBaseOffset(0)
	, fDirty(false)
{
}

template<typename DD>
CIndexPage<DD>::CIndexPage(HStreamBase& inFile, int64 inBaseOffset)
	: fFile(&inFile)
	, fBaseOffset(inBaseOffset)
	, fDirty(false)
{
	Allocate();
}

template<typename DD>
CIndexPage<DD>::CIndexPage(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset)
	: fFile(&inFile)
	, fBaseOffset(inBaseOffset)
	, fOffset(inOffset)
	, fDirty(false)
{
	Read();
}

template<typename DD>
CIndexPage<DD>::~CIndexPage()
{
	if (fDirty)
		Write();
}

template<typename DD>
void CIndexPage<DD>::Load(HStreamBase& inFile, int64 inBaseOffset, uint32 inOffset)
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

template<typename DD>
void CIndexPage<DD>::Allocate()
{
	if (fDirty)
		Write();
	
	memset(&fData, 0, kPageSize);

	fFile->Seek(0, SEEK_END);
	fOffset = DD::PageAddrToNr(fFile->Tell() - fBaseOffset);
	if (fOffset == 0) // avoid returning a zero address!!!
	{
		char c = 0xff;
		fFile->Write(&c, 1);
		fOffset = DD::PageAddrToNr(fFile->Tell() - fBaseOffset);
		assert(fOffset == 1);
	}

	// allocate on disk	
	Write();
}

template<typename DD>
unsigned char* CIndexPage<DD>::GetKey(uint32 inIndex)
{
	if (inIndex > fData.n)
		THROW(("Key index out of range"));
	
	unsigned char* k = fData.keys;
	while (inIndex-- > 0)
		k += k[0] + 1;
	
	return k;
}

template<typename DD>
const unsigned char* CIndexPage<DD>::GetKey(uint32 inIndex) const
{
	if (inIndex > fData.n)
		THROW(("Key index out of range"));
	
	const unsigned char* k = fData.keys;
	while (inIndex-- > 0)
		k += k[0] + 1;
	
	return k;
}

template<typename DD>
void CIndexPage<DD>::GetData(uint32 inIndex, string& outKey, int64& outValue, uint32& outP) const
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	const unsigned char* k = GetKey(inIndex);
	outKey.assign(reinterpret_cast<const char*>(k) + 1, k[0]);
//	outValue = fData.e[-static_cast<int32>(inIndex)].value;
//	outP = fData.e[-static_cast<int32>(inIndex)].p;
	outValue = fData.GetValue(inIndex);
	outP = fData.GetP(inIndex);
}

template<typename DD>
uint32 CIndexPage<DD>::GetP(uint32 inIndex) const
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	return fData.GetP(inIndex);
}

template<typename DD>
uint32 CIndexPage<DD>::GetIndexForP(uint32 inP) const
{
	uint32 ix = 0;

	while (ix < fData.n)
	{
		if (fData.GetP(ix) == inP)
			break;
		++ix;
	}

	return ix;
}

template<typename DD>
void CIndexPage<DD>::SetValue(uint32 inIndex, uint32 inValue)
{
	assert(inIndex < fData.n);
	if (inIndex >= fData.n)
		THROW(("Index out of range"));
	
	fData.SetValue(inIndex, inValue);
	fDirty = true;
}

template<typename DD>
void CIndexPage<DD>::SetParent(uint32 inParent)
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
		if (fData.GetP(i) != 0)
		{
			CIndexPage p(*fFile, fBaseOffset, fData.GetP(i));
			p.SetParent(fOffset);
		}
	}
}

template<>
void CIndexPage<COnDiskDataV0>::SetParent(uint32 inParent)
{
}

template<typename DD>
inline
uint32 CIndexPage<DD>::FreeSpace() const
{
	uint32 result = static_cast<uint32>(kKeySpace - fData.n * kEntrySize);

	const unsigned char* k = GetKey(fData.n);
	result -= k - fData.keys;
	
	return result;
}

template<typename DD>
inline
bool CIndexPage<DD>::CanStore(const string& inKey) const
{
	return FreeSpace() >= inKey.length() + kEntrySize + 1;
}

template<typename DD>
void CIndexPage<DD>::Insert(int32 inIndex, const string& inKey, int64 inValue, uint32 inP)
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
	
	fData.SetValue(inIndex, inValue);
	fData.SetP(inIndex, inP);
	++fData.n;

//#if P_DEBUG
//	cerr << "dumping page " << fOffset << ", parent=" << GetParent() << ", n=" << fData.n << endl;
//
//	for (uint32 i = 0; i < fData.n; ++i)
//	{
//		string k;
//		int64 v;
//		uint32 c;
//		
//		GetData(i, k, v, c);
//		
//		cerr << "ix: " << i << ", p: " << c << ", key: " << k << endl;
//	}
//#endif
}

template<typename DD>
void CIndexPage<DD>::Delete(int32 inIndex)
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

template<typename DD>
void CIndexPage<DD>::Copy(CIndexPage& inFromPage, int32 inFromIndex, int32 inToIndex, int32 inCount)
{
	fDirty = true;

	while (inCount-- > 0)
	{
		assert(inFromIndex < inFromPage.fData.n);
		assert(inToIndex <= fData.n);
		
		string key;
		int64 value;
		uint32 p;
		
		inFromPage.GetData(inFromIndex, key, value, p);
		
		Insert(inToIndex, key, value, p);
		
		++inFromIndex;
		++inToIndex;
	}
}

template<typename DD>
void CIndexPage<DD>::Read()
{
	uint32 read = fFile->PRead(&fData, kPageSize, fBaseOffset + DD::PageNrToAddr(fOffset));
	assert(read == kPageSize);
	if (read != kPageSize)
		THROW(("IO Error, reading page for index"));
	
	fDirty = false;

	if (fFile->SwapsBytes())
		fData.SwapBytes();
}

template<typename DD>
void CIndexPage<DD>::Write()
{
	uint32 written = fFile->PWrite(&fData, kPageSize, fBaseOffset + DD::PageNrToAddr(fOffset));
	assert(written == kPageSize);
	if (written != kPageSize)
		THROW(("IO Error, writing page for index"));
	
	fDirty = false;
}

// ---------------------------------------------------------------------------
// 
// iterator_imp
// 

struct iterator_imp
{
	virtual				~iterator_imp() {}
	
						iterator_imp(HStreamBase& inFile, int64 inOffset, uint32 inRoot);
						iterator_imp(const iterator_imp& inOther);
						
	virtual void		begin() = 0;
	virtual void		end() = 0;
	virtual void		walk_down_path(const stack<uint32>& inPath, uint32 inIndex) = 0;

	virtual void		increment() = 0;
	virtual void		decrement() = 0;
	
	virtual iterator_imp*
						clone() const = 0;
	
	bool				equal(const iterator_imp& inOther) const;
	const pair<string,int64>&
						dereference() const;

	HStreamBase*		fFile;
	int64				fBaseOffset;
	uint32				fRoot;
	uint32				fPage;
	uint32				fPageIndex;
	pair<string,int64>	fCurrent;
};

iterator_imp::iterator_imp(HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: fFile(&inFile)
	, fBaseOffset(inOffset)
	, fRoot(inRoot)
	, fPage(0)
	, fPageIndex(0)
{
}

iterator_imp::iterator_imp(const iterator_imp& inOther)
	: fFile(inOther.fFile)
	, fBaseOffset(inOther.fBaseOffset)
	, fRoot(inOther.fRoot)
	, fPage(inOther.fPage)
	, fPageIndex(inOther.fPageIndex)
	, fCurrent(inOther.fCurrent)
{
}

bool iterator_imp::equal(const iterator_imp& inOther) const
{
	assert(fFile == inOther.fFile);
	assert(fBaseOffset == inOther.fBaseOffset);

//cout << "Comparing: " << endl;
//cout << "\tpage: " << fPage << " <=> " << inOther.fPage << endl;
//cout << "\tindx: " << fPageIndex << " <=> " << fPageIndex << endl;

	return
		fFile == inOther.fFile and fBaseOffset == inOther.fBaseOffset and
		fRoot == inOther.fRoot and
		fPage == inOther.fPage and fPageIndex == inOther.fPageIndex;
}

inline
const pair<string,int64>&
iterator_imp::dereference() const
{
	return fCurrent;
}

// ---------------------------------------------------------------------------
// 
// iterator_imp_t
// 

template<class DD>
struct iterator_imp_t : public iterator_imp
{
	typedef DD				COnDiskData;
	typedef CIndexPage<DD>	CIndexPage;

							iterator_imp_t(HStreamBase& inFile, int64 inOffset, uint32 inRoot);
							iterator_imp_t(const iterator_imp_t& inOther);

	virtual void			begin();
	virtual void			end();
	virtual void			walk_down_path(const stack<uint32>& inPath, uint32 inIndex);

	virtual iterator_imp*	clone() const;

	virtual void			increment();
	virtual void			decrement();

	stack<uint32>			fStack;
	CIndexPage				fDiskPage;
};

// ---------------------------------------------------------------------------
// 
// CIndexImp
// 

struct CIndexImp
{
	typedef CIndex::iterator	iterator;
	
					CIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot);
	virtual			~CIndexImp() {}

	HStreamBase&	GetFile() const					{ return fFile; }
	int64			GetBaseOffset() const			{ return fBaseOffset; }
	uint32			GetRoot() const					{ return fRoot; }
	uint32			GetKind() const					{ return fKind; }

	virtual iterator
					Begin() = 0;		// Begin is defined as the first entry on the left most leaf page
	virtual iterator
					End() = 0;			// End is defined as N on the right most leaf page
	virtual iterator
					LowerBound(const string& inKey) = 0;

	virtual uint32	GetCount(uint32 inPage) const = 0;

	virtual bool	GetValue(const string& inKey, int64& outValue) const = 0;

	virtual void	Visit(uint32 inPage, CIndex::VisitorBase& inVisitor) = 0;
	virtual void	VisitForPattern(const string& inPattern, uint32 inPage,
						CIndex::VisitorBase& inVisitor) = 0;

	virtual void	CreateFromIterator(CIteratorBase& inIter) = 0;

#if P_DEBUG
	virtual void	Test(CIndex& inIndex) = 0;
	virtual void	Dump(uint32 inPage, uint32 inParent, uint32 inLevel) = 0;
#endif

 	virtual int		Compare(const char* inA, uint32 inLengthA,
						const char* inB, uint32 inLengthB) const;
	int				Compare(const string& inA, const string& inB) const
						{ return Compare(inA.c_str(), inA.length(), inB.c_str(), inB.length()); }

	void			ValueAccumulator(const string& inKey, int64 inValue);

	HStreamBase&	fFile;
	int64			fBaseOffset;
	uint32			fRoot;
	uint32			fKind;
	vector<uint32>*	fValues;
};

CIndexImp::CIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: fFile(inFile)
	, fBaseOffset(inOffset)
	, fRoot(inRoot)
	, fKind(inKind)
	, fValues(nil)
{
}

int CIndexImp::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	return CompareKeyString(inA, inLengthA, inB, inLengthB);
}

void CIndexImp::ValueAccumulator(const string& inKey, int64 inValue)
{
	fValues->push_back(inValue);
	push_heap(fValues->begin(), fValues->end());
}

// ---------------------------------------------------------------------------
// 
// CIndexImpT
// 

template<typename DD>
struct CIndexImpT : public CIndexImp
{
	typedef DD				COnDiskData;
	typedef CIndexPage<DD>	CIndexPage;

  public:
					CIndexImpT(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot);

	virtual iterator
					Begin();		// Begin is defined as the first entry on the left most leaf page
	virtual iterator
					End();			// End is defined as N on the right most leaf page
	virtual iterator
					LowerBound(const string& inKey);

	virtual bool	GetValue(const string& inKey, int64& outValue) const;

	virtual uint32	GetCount(uint32 inPage) const;

	virtual void	Visit(uint32 inPage, CIndex::VisitorBase& inVisitor);
	virtual void	VisitForPattern(
						const string&			inPattern,
						uint32					inPage,
						CIndex::VisitorBase&	inVisitor);

	virtual void	CreateFromIterator(CIteratorBase& inIter);

#if P_DEBUG
	virtual void	Test(CIndex& inIndex);
	virtual void	Dump(uint32 inPage, uint32 inParent, uint32 inLevel);
#endif

  private:

	void			LowerBoundImp(const string& inKey, uint32 inPage, stack<uint32>& outPages, uint32& outIndex);
};

template<typename DD>
CIndexImpT<DD>::CIndexImpT(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: CIndexImp(inKind, inFile, inOffset, inRoot)
{
}

template<typename DD>
CIndex::iterator CIndexImpT<DD>::Begin()
{
	auto_ptr<iterator_imp> result(new iterator_imp_t<DD>(fFile, fBaseOffset, fRoot));
	result->begin();
	return result.release();
}

template<typename DD>
CIndex::iterator CIndexImpT<DD>::End()
{
	auto_ptr<iterator_imp> result(new iterator_imp_t<DD>(fFile, fBaseOffset, fRoot));
	result->end();
	return result.release();
}

template<typename DD>
void CIndexImpT<DD>::LowerBoundImp(const string& inKey, uint32 inPage, stack<uint32>& outPages, uint32& outIndex)
{
	if (inPage == 0)
		return;

	outPages.push(inPage);

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
		outIndex = static_cast<uint32>(R + 1);
		
		uint32 p;
		if (R >= 0)
			p = data.GetP(R);
		else
			p = data.p0;

		if (p != 0)
			LowerBoundImp(inKey, p, outPages, outIndex);
	}
}

template<typename DD>
CIndex::iterator CIndexImpT<DD>::LowerBound(const string& inKey)
{
	uint32 index = 0;
	stack<uint32> pages;

	LowerBoundImp(inKey, fRoot, pages, index);
	
	auto_ptr<iterator_imp> result(new iterator_imp_t<DD>(fFile, fBaseOffset, fRoot));
	result->walk_down_path(pages, index);
	return result.release();
}

template<typename DD>
bool CIndexImpT<DD>::GetValue(const string& inKey, int64& outValue) const
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
				outValue = data.GetValue(i);
				break;
			}
			else if (d < 0)
				R = i - 1;
			else
				L = i + 1;
		}
		
		if (R >= 0)
			page = data.GetP(R);
		else
			page = data.p0;
	}
	while (not found and page != 0);

	return found;
}

template<typename DD>
uint32 CIndexImpT<DD>::GetCount(uint32 inPage) const
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

template<typename DD>
void CIndexImpT<DD>::Visit(uint32 inPage, CIndex::VisitorBase& inVisitor)
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
			uint32 c;
			int64 v;
			
			p.GetData(i, k, v, c);

			inVisitor.Visit(k, v);
			
			if (p.GetP(i))
				Visit(p.GetP(i), inVisitor);
		}
	}
}

template<typename DD>
void CIndexImpT<DD>::VisitForPattern(
	const string&			inPattern,
	uint32					inPage,
	CIndex::VisitorBase&	inVisitor)
{
	uint32 page = fRoot;
	
	pair<uint32,uint32> v(page, 0);
	stack<pair<uint32,uint32> > pStack;

	uint32 c;
	int64 val;
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
		
			MatchResult m = Match(inPattern.c_str(), key.c_str());

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
			page = p.GetData().GetP(R);
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
			
			MatchResult m = Match(inPattern.c_str(), key.c_str());

			if (m == eMatch)
			{
				if (val >= numeric_limits<uint32>::max())
					THROW(("index value out of range"));
				
				inVisitor.Visit(key, val);
			}
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

struct CTempValue
{
	string		key;
	uint32		value;
	uint32		pageLeft;
	uint32		pageRight;
};

typedef list<CTempValue>	CTempValueList;

template<typename DD>
void CIndexImpT<DD>::CreateFromIterator(CIteratorBase& inData)
{
	assert(sizeof(DD) == kPageSize);
	
	// first pass, collect the data from the iterator building the leaf pages
	
	string k, lk;
	int64 v;
	
	CIndexPage p(fFile, fBaseOffset);
	CTempValueList up;

	fRoot = p.GetOffset();

	uint32 n = 0;

	while (inData.Next(k, v))
	{
		++n;
		
		if (k.length() > kMaxKeySize)
		{
			THROW(("Attempt to enter key with length larger than %d in index, this key will be skipped\n'%s'",
				kMaxKeySize, k.c_str()));
		}
		
		if (lk.length() and Compare(k, lk) <= 0)
		{
			THROW(("Attempt to build an index from unsorted data: '%s' <= '%s' ",
				k.c_str(), lk.c_str()));
		}
		lk = k;
		
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
			up.back().pageRight = p.GetData().p0;
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
template<typename DD>
void CIndexImpT<DD>::Test(CIndex& inIndex)
{
	vector<string> keys;
	
	for (iterator i = Begin(); i != End(); ++i)
	{
		assert(keys.size() == 0 or Compare(keys.back(), i->first) < 0);
		keys.push_back(i->first);
	}
	
	for (vector<string>::iterator k = keys.begin(); k != keys.end(); ++k)
	{
		iterator i = inIndex.find(*k);
		
		assert(i != inIndex.end());
		assert(i->first == *k);
	}
	
	uint32 cnt = inIndex.GetCount(), n = 0;
	for (iterator i = inIndex.begin(), end(inIndex.end()); i != end; ++i, ++n)
		;

	assert(n == cnt);
	
	n = 0;
	for (iterator i = inIndex.end(), begin(inIndex.begin()); i != begin; --i, ++n)
		;

	assert(n == cnt);
}

template<typename DD>
void CIndexImpT<DD>::Dump(uint32 inPage, uint32 inParent, uint32 inLevel)
{
	const CIndexPage p(fFile, fBaseOffset, inPage);

	for (uint32 j = 0; j < inLevel; ++j)
		cout << "\t";
	cout << "dumping page " << inPage << ", parent = " << p.GetParent() << endl;

	if (inParent != p.GetParent())
		cout << "!!! Parent is not correct, should be: " << inParent << endl;

//	for (uint32 i = 0; i < p.GetN(); ++i)
//	{
//		for (uint32 j = 0; j < inLevel; ++j)
//			cout << "\t";
//		
//		string k;
//		int64 v;
//		uint32 c;
//		
//		p.GetData(i, k, v, c);
//		
//		cout << "ix: " << i << ", p: " << c << ", key: " << k << " v: " << v << endl;
//	}

	if (p.GetP0())
	
		Dump(p.GetP0(), inPage, inLevel + 1);
	
	for (uint32 i = 0; i < p.GetN(); ++i)
	{
		string k;
		int64 v;
		uint32 c;
		
		p.GetData(i, k, v, c);
		
		if (c != 0)
			Dump(c, inPage, inLevel + 1);
	}

	cout << endl;
}
#endif

// ---------------------------------------------------------------------------

template<typename DD>
class CNumberIndexImp : public CIndexImpT<DD>
{
  public:
					CNumberIndexImp(uint32 inKind, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
						: CIndexImpT<DD>(inKind, inFile, inOffset, inRoot) {}

	virtual int		Compare(const char* inA, uint32 inLengthA,
						const char* inB, uint32 inLengthB) const;
};

template<typename DD>
int CNumberIndexImp<DD>::Compare(const char* inA, uint32 inLengthA,
	const char* inB, uint32 inLengthB) const
{
	return CompareKeyNumber(inA, inLengthA, inB, inLengthB);
}

// ---------------------------------------------------------------------------

// normal constructor for an exisiting index on disk
CIndex::CIndex(uint32 inKind, CIndexVersion inVersion, HStreamBase& inFile, int64 inOffset, uint32 inRoot)
{
	switch (inKind)
	{
		case kTextIndex:
		case kDateIndex:
		case kValueIndex:
		case kWeightedIndex:
			switch (inVersion)
			{
				case kCIndexVersionV0:
					fImpl = new CIndexImpT<COnDiskDataV0>(inKind, inFile, inOffset, inRoot);
					break;

				case kCIndexVersionV1:
					if (inFile.SwapsBytes())
						fImpl = new CIndexImpT<COnDiskDataV1S>(inKind, inFile, inOffset, inRoot);
					else
						fImpl = new CIndexImpT<COnDiskDataV1N>(inKind, inFile, inOffset, inRoot);
					break;
				
				case kCIndexVersionV2:
					if (inFile.SwapsBytes())
						fImpl = new CIndexImpT<COnDiskDataV2S>(inKind, inFile, inOffset, inRoot);
					else
						fImpl = new CIndexImpT<COnDiskDataV2N>(inKind, inFile, inOffset, inRoot);
					break;

				default:
					THROW(("Invalid index version"));
					break;
			}
			break;
		
		case kNumberIndex:
			switch (inVersion)
			{
				case kCIndexVersionV0:
					fImpl = new CNumberIndexImp<COnDiskDataV0>(inKind, inFile, inOffset, inRoot);
					break;

				case kCIndexVersionV1:
					if (inFile.SwapsBytes())
						fImpl = new CNumberIndexImp<COnDiskDataV1S>(inKind, inFile, inOffset, inRoot);
					else
						fImpl = new CNumberIndexImp<COnDiskDataV1N>(inKind, inFile, inOffset, inRoot);
					break;
				
				case kCIndexVersionV2:
					if (inFile.SwapsBytes())
						fImpl = new CNumberIndexImp<COnDiskDataV2S>(inKind, inFile, inOffset, inRoot);
					else
						fImpl = new CNumberIndexImp<COnDiskDataV2N>(inKind, inFile, inOffset, inRoot);
					break;

				default:
					THROW(("Invalid index version"));
					break;
			}
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

// copy creates a compacted copy
CIndex* CIndex::CreateFromIterator(uint32 inIndexKind, CIndexVersion inVersion, CIteratorBase& inData, HStreamBase& inFile)
{
	int64 offset = inFile.Seek(0, SEEK_END);

		// align the pages on disk
	if (inVersion == kCIndexVersionV2)
	{
		if ((offset % kPageSize) != 0)
		{
			char b[kPageSize] = { };
			
			offset += inFile.Write(b, kPageSize - (offset % kPageSize));
			assert((offset % kPageSize) == 0);
		}
	}
	else
	{
		if (((offset + 1) % kPageSize) != 0)
		{
			char b[kPageSize] = { };
			
			offset += inFile.Write(b, kPageSize - ((offset + 1) % kPageSize));
			assert(((offset + 1) % kPageSize) == 0);
		}
	}

	auto_ptr<CIndex> result(new CIndex(inIndexKind, inVersion, inFile, offset, 0));
	result->fImpl->CreateFromIterator(inData);
	
	inFile.Seek(0, SEEK_END);
	
//#if P_DEBUG
//	result->fImpl->Test(*result);
////	result->Dump();
//#endif
	
	return result.release();
}

int64 CIndex::GetOffset() const
{
	return fImpl->GetBaseOffset();
}

// access data
bool CIndex::GetValue(const string& inKey, int64& outValue) const
{
	return fImpl->GetValue(inKey, outValue);
}

bool CIndex::GetValue(const string& inKey, uint32& outValue) const
{
	int64 v;
	bool result = false;
	
	if (GetValue(inKey, v))
	{
		if (v >= numeric_limits<uint32>::max())
			THROW(("Index value out of range"));

		outValue = static_cast<uint32>(v);
		result = true;
	}
	
	return result;
}

void CIndex::GetValuesForPattern(const string& inKey, std::vector<uint32>& outValues)
{
	fImpl->fValues = &outValues;
	VisitForPattern(inKey, fImpl, &CIndexImp::ValueAccumulator);
	fImpl->fValues = nil;
	sort_heap(outValues.begin(), outValues.end());
}

void CIndex::GetValuesForRange(const char* inLowerBound,
	const char* inUpperBound, std::vector<uint32>& outValues)
{
	iterator b, e, i;
	
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
	iterator b, e, i;
	
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

void CIndex::VisitForPattern(const string& inPattern, VisitorBase& inVisitor)
{
	fImpl->VisitForPattern(inPattern, fImpl->GetRoot(), inVisitor);
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
	fImpl->Dump(fImpl->GetRoot(), 0, 0);
}
#endif

// ---------------------------------------------------------------------------
//
// iterator	(boost rules!)
//

template<typename DD>
iterator_imp_t<DD>::iterator_imp_t(HStreamBase& inFile, int64 inOffset, uint32 inRoot)
	: iterator_imp(inFile, inOffset, inRoot)
{
}

template<typename DD>
iterator_imp_t<DD>::iterator_imp_t(const iterator_imp_t& inOther)
	: iterator_imp(inOther)
	, fStack(inOther.fStack)
	, fDiskPage(inOther.fDiskPage)
{
}

template<typename DD>
void iterator_imp_t<DD>::begin()
{
	fPage = fRoot;
	fDiskPage.Load(*fFile, fBaseOffset, fPage);
	fPageIndex = 0;

	while (fDiskPage.GetP0() != 0)
	{
		fStack.push(fPage);
		fPage = fDiskPage.GetP0();
		fDiskPage.Load(*fFile, fBaseOffset, fPage);
	}

	if (fPageIndex < fDiskPage.GetN())
	{
		uint32 c;
		fDiskPage.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
	}
}

template<typename DD>
void iterator_imp_t<DD>::end()
{
	fPage = fRoot;
	fDiskPage.Load(*fFile, fBaseOffset, fPage);
	fPageIndex = fDiskPage.GetN();

	while (fDiskPage.GetP(fPageIndex - 1) != 0)
	{
		fStack.push(fPage);
		fPage = fDiskPage.GetP(fPageIndex - 1);
		fDiskPage.Load(*fFile, fBaseOffset, fPage);
		fPageIndex = fDiskPage.GetN();
	}
}

template<typename DD>
void iterator_imp_t<DD>::walk_down_path(const stack<uint32>& inPages, uint32 inIndex)
{
	assert(not inPages.empty());
	
	fStack = inPages;
	fPage = fStack.top();
	fStack.pop();
	fPageIndex = inIndex;
	
	fDiskPage.Load(*fFile, fBaseOffset, fPage);
	
	if (fPageIndex < fDiskPage.GetN())
	{
		uint32 c;
		fDiskPage.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
	}
}

template<typename DD>
void iterator_imp_t<DD>::increment()
{
	bool valid = true;

	if (fPageIndex < fDiskPage.GetN() and fDiskPage.GetP(fPageIndex)) // current entry has a p, decent
	{
		fStack.push(fPage);
		
		fPage = fDiskPage.GetP(fPageIndex);
		fPageIndex = 0;

		fDiskPage.Load(*fFile, fBaseOffset, fPage);
		
		while (fDiskPage.GetP0())
		{
			fStack.push(fPage);
			
			fPage = fDiskPage.GetP0();
			fDiskPage.Load(*fFile, fBaseOffset, fPage);
		}
	}
	else
	{
		++fPageIndex;
		
		if (fPageIndex >= fDiskPage.GetN())		// we're running off this page
		{										// see if there's a page right to us
			while (fPageIndex >= fDiskPage.GetN() and not fStack.empty())
			{
				fDiskPage.Load(*fFile, fBaseOffset, fStack.top());
				fStack.pop();
				
				if (fDiskPage.GetP0() == fPage)
					fPageIndex = 0;
				else
					fPageIndex = fDiskPage.GetIndexForP(fPage) + 1;
				
				fPage = fDiskPage.GetOffset();
			}
			
			valid = fPageIndex < fDiskPage.GetN();
		}
	}
	
	if (valid)
	{
		uint32 c;
		fDiskPage.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
	}
	else
		end();
}

template<typename DD>
void iterator_imp_t<DD>::decrement()
{
	uint32 dp = 0;

	if (fPageIndex == 0)		// beginning of page
	{
		if (fDiskPage.GetP0())			// see if we need to decent down p0 first
			dp = fDiskPage.GetP0();
		else
		{
			uint32 np = fPage;		// tricky, see if we can find a page upwards
			uint32 nx = fPageIndex;	// where we hit an index > 0. If not we're at
									// the far left and thus Begin()
			
			stack<uint32> ps(fStack);
			
			while (not ps.empty())
			{
				fDiskPage.Load(*fFile, fBaseOffset, ps.top());
				ps.pop();
				
				if (np == fDiskPage.GetP0())
				{
					np = fDiskPage.GetOffset();
					nx = 0;
				}
				else
				{
					fPageIndex = fDiskPage.GetIndexForP(np);
					fPage = fDiskPage.GetOffset();
					fStack = ps;
					break;
				}
			}
		}
	}
	else
	{
		--fPageIndex;
		dp = fDiskPage.GetP(fPageIndex);
	}
	
	while (dp)
	{
		fDiskPage.Load(*fFile, fBaseOffset, dp);
		fStack.push(fPage);
		fPage = dp;
		fPageIndex = fDiskPage.GetN() - 1;
		dp = fDiskPage.GetP(fPageIndex);
	}
	
	fDiskPage.Load(*fFile, fBaseOffset, fPage);
	uint32 c;
	fDiskPage.GetData(fPageIndex, fCurrent.first, fCurrent.second, c);
}

template<typename DD>
iterator_imp* iterator_imp_t<DD>::clone() const
{
	return new iterator_imp_t<DD>(*this);
}

// ---------------------------------------------------------------------------

CIndex::iterator::iterator()
	: fImpl(nil)
{
}

CIndex::iterator::~iterator()
{
	delete fImpl;
}

CIndex::iterator::iterator(iterator_imp* inImpl)
	: fImpl(inImpl)
{
}

CIndex::iterator::iterator(const iterator& inOther)
	: fImpl(nil)
{
	iterator_imp* imp = inOther.fImpl;
	if (imp != nil)
		fImpl = imp->clone();
}

CIndex::iterator& CIndex::iterator::operator=(const iterator& inOther)
{
	if (this != &inOther)
	{
		delete fImpl;
		
		if (inOther.fImpl != nil)
			fImpl = inOther.fImpl->clone();
		else
			fImpl = nil;
	}

	return *this;
}

void CIndex::iterator::increment()
{
	if (fImpl == nil)
		THROW(("Uninitialised iterator"));
	
	fImpl->increment();
}

void CIndex::iterator::decrement()
{
	if (fImpl == nil)
		THROW(("Uninitialised iterator"));
	
	fImpl->decrement();
}

bool CIndex::iterator::equal(const iterator& inOther) const
{
	if (fImpl == nil or inOther.fImpl == nil)
		THROW(("Uninitialised iterator"));
	
	return fImpl->equal(*inOther.fImpl);
}

CIndex::iterator::reference CIndex::iterator::dereference() const
{
	if (fImpl == nil)
		THROW(("Uninitialised iterator"));
	
	return fImpl->dereference();
}

