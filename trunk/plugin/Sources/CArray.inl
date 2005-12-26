/*	$Id: CArray.inl,v 1.21 2005/08/22 12:38:06 maarten Exp $
	Copyright Maarten L. Hekkelman
	Created Monday January 27 2003 20:19:40
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
 
#include "HStream.h"
#include "HStlIOStream.h"
#include "HError.h"

template<class T>
const int CCArray<T>::kPageSize = 256;

template<class T>
CCArray<T>::CCArray(const std::vector<T>& inData, int64 inMax)
	: cnt(inData.size())
	, data(NULL)
	, table(NULL)
{
	Init(inMax);
	
	std::vector<char> buf;
	CreatePage(inData, 0, (cnt - 1) / kPageSize, buf);
	
	obit_stream bits;
	WriteGamma(bits, cnt);
	bits.sync();
	
	size = bits.size() + buf.size();
	data = new char[size];
	memcpy(data, bits.peek(), bits.size());
	
	data_offset = bits.size();
	
	copy(buf.begin(), buf.end(), data + data_offset);
}

template<class T>
CCArray<T>::CCArray(HStreamBase& inData, int64 inMax)
	: cnt(inData.Size() / sizeof(T))
	, data(NULL)
	, table(NULL)
{
	Init(inMax);
	
	std::vector<char> buf;
	CreatePage(inData, 0, (cnt - 1) / kPageSize, buf);
	
	obit_stream bits;
	WriteGamma(bits, cnt);
	bits.sync();
	
	size = bits.size() + buf.size();
	data = new char[size];
	memcpy(data, bits.peek(), bits.size());
	
	data_offset = bits.size();
	
	copy(buf.begin(), buf.end(), data + data_offset);
}

template<class T>
CCArray<T>::CCArray(HStreamBase& inFile, int64 inOffset, uint32 inSize, int64 inMax)
	: size(inSize)
	, data(new char[inSize])
	, table(NULL)
{
	inFile.PRead(data, size, inOffset);
	
	ibit_stream bits(data, size);
	cnt = ReadGamma(bits);
	data_offset = bits.bytes_read();

	Init(inMax);
}

template<class T>
CCArray<T>::~CCArray()
{
	delete[] data;
	delete[] table;
}

template<class T>
void CCArray<T>::Init(int64 inMax)
{
	int64 v = inMax;
	
	kb = 0;
	
	while (v)
	{
		++kb;
		v >>= 1;
	}
	
	b = CalculateB(inMax, cnt);
	n = 0;
	g = 1;
	
	while (g < b)
	{
		++n;
		g <<= 1;
	}

	g -= b;
}

template<class T>
void CCArray<T>::CreatePage(const T* inData, uint32 inDataSize,
	std::vector<char>& ldata, std::vector<char>& rdata,
	std::vector<char>& outData)
{
	uint32 e = 0;
	uint32 m = kPageSize;
	if (m > inDataSize)
		m = inDataSize;
	
	T v = inData[e];

	obit_stream bits;
	
	for (int i = kb - 1; i >= 0; --i)
		bits << (((1ULL << i) & v) != 0);

	// too bad, we have to determine first how long our array will be
	uint32 l = 0;

	for (++e; e < m; ++e)
	{
		int32 d = static_cast<int32>(inData[e] - v);
		assert(d > 0);
		
		int32 q = (d - 1) / b;
		int32 r = d - q * b - 1;
		
		l += static_cast<uint32>(q + 1);
		
		if (b > 1)
		{
			if (r < g)
				l += static_cast<uint32>(n - 1);
			else
				l += static_cast<uint32>(n + 0);
		}
		
		v = inData[e];
	}
	
	l = l / 8 + 1;

	WriteGamma(bits, l);
	WriteGamma(bits, l + ldata.size());
	l += bits.size() + 1;
	
//	e = ix * kPageSize;
	e = 0;
	v = inData[e];

	for (++e; e < m; ++e)
	{
		int32 d = static_cast<int32>(inData[e] - v);
		assert(d > 0);
		
		int32 q = (d - 1) / b;
		int32 r = d - q * b - 1;
		
		while (q-- > 0)
			bits << 1;
		bits << 0;
		
		if (b > 1)
		{
			if (r < g)
			{
				for (int t = 1 << (n - 2); t != 0; t >>= 1)
					bits << ((r & t) != 0);
			}		
			else
			{
				r += g;
				for (int t = 1 << (n - 1); t != 0; t >>= 1)
					bits << ((r & t) != 0);
			}
		}
		
		v = inData[e];
	}
	
	bits.sync();
	
	std::back_insert_iterator<std::vector<char> > iter(outData);

	const char* p = bits.peek();
	std::copy(p, p + bits.size(), iter);

	uint32 s = bits.size();
	while (s < l)	// fill with zero's if needed
	{
		*iter++ = 0;
		++s;
	}
	
	assert(s == l);

	std::copy(ldata.begin(), ldata.end(), iter);
	std::copy(rdata.begin(), rdata.end(), iter);
}

template<class T>
void CCArray<T>::CreatePage(HStreamBase& inData,
	uint32 inLeft, uint32 inRight, std::vector<char>& outData)
{
	std::vector<char> ldata, rdata;
	
	uint32 ix = (inLeft + inRight) / 2;
	
	if (ix > inLeft)
		CreatePage(inData, inLeft, ix - 1, ldata);

	if (ix < inRight)
		CreatePage(inData, ix + 1, inRight, rdata);
	
	uint32 e = ix * kPageSize;
	uint32 m = (ix + 1) * kPageSize;
	if (m > inData.Size() / sizeof(T))
		m = inData.Size() / sizeof(T);
	
	HAutoBuf<T> d(new T[m - e]);
	int64 offset = e * sizeof(T);
	inData.PRead(d.get(), (m - e) * sizeof(T), offset);
	
	CreatePage(d.get(), m - e, ldata, rdata, outData);
}

template<class T>
void CCArray<T>::CreatePage(const std::vector<T>& inData,
	uint32 inLeft, uint32 inRight, std::vector<char>& outData)
{
	std::vector<char> ldata, rdata;
	
	uint32 ix = (inLeft + inRight) / 2;
	
	if (ix > inLeft)
		CreatePage(inData, inLeft, ix - 1, ldata);

	if (ix < inRight)
		CreatePage(inData, ix + 1, inRight, rdata);
	
	uint32 e = ix * kPageSize;
	uint32 m = (ix + 1) * kPageSize;
	if (m > inData.size())
		m = inData.size();
	
	HAutoBuf<T> d(new T[m - e]);
	std::copy(inData.begin() + e, inData.begin() + m, d.get());
	
	CreatePage(d.get(), m - e, ldata, rdata, outData);
}

template<class T>
T CCArray<T>::operator[](uint32 inIndex) const
{
	if (table != nil)
		return table[inIndex];
	
	uint32 offset = data_offset;
	
	uint32 L = 0, R = (cnt - 1) / kPageSize;
	uint32 k = inIndex / kPageSize;
	T result;
	
	while (L <= R)
	{
		ibit_stream bits(data + offset, 0);
		
		result = 0;
		for (int i = kb - 1; i >= 0; --i)
		{
			if (bits.next_bit())
				result |= (1ULL << i);
		}
		
		uint32 l, r;
		l = ReadGamma(bits);
		r = ReadGamma(bits);

		uint32 ix = (L + R) / 2;
		if (k < ix)
		{
			R = ix - 1;
			offset += bits.bytes_read() + l;
		}
		else if (k > ix)
		{
			L = ix + 1;
			offset += bits.bytes_read() + r;
		}
		else
		{
			uint32 e = inIndex - ix * kPageSize;
			while (e-- > 0)
			{
				int32 q = 0;
				int32 r = 0;
				
				if (bits.next_bit())
				{
					q = 1;
					while (bits.next_bit())
						++q;
				}
				
				if (b > 1)
				{
					for (int e = 0; e < n - 1; ++e)
						r = (r << 1) | bits.next_bit();
					
					if (r >= g)
					{
						r = (r << 1) | bits.next_bit();
						r -= g;
					}
				}
				
				result += r + q * b + 1;
			}
				
			break;
		}
	}
	
	return result;
}

template<class T>
void CCArray<T>::ExpandPage(uint32 inL, uint32 inR, uint32 inPage, uint32 inOffset)
{
	ibit_stream bits(data + inOffset, 0);

	T base = 0;
	for (int i = kb - 1; i >= 0; --i)
	{
		if (bits.next_bit())
			base |= (1ULL << i);
	}
	
	uint32 l, r;
	l = ReadGamma(bits);
	r = ReadGamma(bits);

	uint32 offset = bits.bytes_read();

	uint32 N = (inPage + 1) * kPageSize;
	if (N > cnt)
		N = cnt;

	table[inPage * kPageSize] = base;
	for (uint32 ix = inPage * kPageSize + 1; ix < N; ++ix)
	{
		int32 q = 0;
		int32 r = 0;
		
		if (bits.next_bit())
		{
			q = 1;
			while (bits.next_bit())
				++q;
		}
		
		if (b > 1)
		{
			for (int e = 0; e < n - 1; ++e)
				r = (r << 1) | bits.next_bit();
			
			if (r >= g)
			{
				r = (r << 1) | bits.next_bit();
				r -= g;
			}
		}
		
		base += r + q * b + 1;
		table[ix] = base;
	}
	
	if (inPage > inL)
		ExpandPage(inL, inPage - 1, (inL + inPage - 1) / 2, inOffset + offset + l);

	if (inPage < inR)
		ExpandPage(inPage + 1, inR, (inPage + 1 + inR) / 2, inOffset + offset + r);
}

template<class T>
void CCArray<T>::Expand()
{
	try
	{
		table = new T[cnt];
		
		uint32 L = 0;
		uint32 R = (cnt - 1) / kPageSize;
		uint32 p = (L + R) / 2;
		
		ExpandPage(L, R, p, data_offset);

#if P_DEBUG
		T* test = table;
		table = NULL;
		for (uint32 i = 0; i < cnt; ++i)
		{
			T a = test[i];
			T b = operator[](i);
			
			if (a != b)
			{
//				THROW(("expanding failed: ix: %d, table: %d, test: %d\n", i, a, b));
//				exit(1);
				std::cerr << "test[" << i << "] = " << a << " table[" << i << "] = " << b << std::endl;
			}
		}
		table = test;
#endif

	}
	catch (...) // silently fail
	{
		table = NULL;
	}
}
