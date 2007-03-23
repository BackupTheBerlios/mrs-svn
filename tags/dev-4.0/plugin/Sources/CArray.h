/*	$Id: CArray.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Monday January 27 2003 20:14:57
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
 
#ifndef CARRAY_H
#define CARRAY_H

#include "CBitStream.h"

class HStreamBase;

template <class T>
class CCArray
{
  public:
	static const int kPageSize;
//	enum { kPageSize = 256 };

						CCArray(const std::vector<T>& inData, int64 inMax);
						CCArray(HStreamBase& inData, int64 inMax);
						CCArray(HStreamBase& inFile, int64 inOffset, uint32 inSize,
							int64 inMax);
						~CCArray();
	
						// sometimes we simply need fast access at the cost of memory
	bool				IsExpanded() const		{ return table != nil; }
	void				Expand();
	
//	bool				Contains(T inValue) const;
	T					operator[] (uint32 inIndex) const;
	
	const void*			Peek() const			{ return data; }
	uint32				Size() const			{ return size; }
	
	
  private:

	void				ExpandPage(uint32 inL, uint32 inR, uint32 inPage, uint32 inOffset);

	void				Init(int64 inMax);
	void				CreatePage(const T* inData, uint32 inDataSize,
							std::vector<char>& ldata, std::vector<char>& rdata,
							std::vector<char>& outData);
	void				CreatePage(const std::vector<T>& inData,
							uint32 inLeft, uint32 inRight,
							std::vector<char>& outData);
	void				CreatePage(HStreamBase& inData,
							uint32 inLeft, uint32 inRight,
							std::vector<char>& outData);

	int32				kb, b, n, g;
	uint32				cnt, data_offset;
	uint32				size;
	char*				data;
	T*					table;
};

#include "CArray.inl"

#endif // CARRAY_H
