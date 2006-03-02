/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Sunday December 02 2001 11:36:08
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
 
#ifndef HBYTESWAP_H
#define HBYTESWAP_H

// helper class, effectively a noop
struct no_swapper
{
	template<typename T>
	static T	swap(T inValue)		{ return inValue; }
};

// a class that swaps
struct byte_swapper
{
	static bool		swap(bool inValue);
	static int8		swap(int8 inValue);
	static int16	swap(int16 inValue);
	static int32	swap(int32 inValue);
	static float	swap(float inValue);
	static int64	swap(int64 inValue);
	static uint8	swap(uint8 inValue);
	static uint16	swap(uint16 inValue);
	static uint32	swap(uint32 inValue);
	static uint64	swap(uint64 inValue);
};

inline
int64 byte_swapper::swap(int64 inValue)
{
	return static_cast<int64>(
		(((static_cast<uint64>(inValue))<<56) & 0xFF00000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<40) & 0x00FF000000000000ULL)  |
		(((static_cast<uint64>(inValue))<<24) & 0x0000FF0000000000ULL)  |
		(((static_cast<uint64>(inValue))<< 8) & 0x000000FF00000000ULL)  |
		(((static_cast<uint64>(inValue))>> 8) & 0x00000000FF000000ULL)  |
		(((static_cast<uint64>(inValue))>>24) & 0x0000000000FF0000ULL)  |
		(((static_cast<uint64>(inValue))>>40) & 0x000000000000FF00ULL)  |
		(((static_cast<uint64>(inValue))>>56) & 0x00000000000000FFULL));
}

inline
uint64 byte_swapper::swap(uint64 inValue)
{
	return static_cast<uint64>(
		((((uint64)inValue)<<56) & 0xFF00000000000000ULL)  |
		((((uint64)inValue)<<40) & 0x00FF000000000000ULL)  |
		((((uint64)inValue)<<24) & 0x0000FF0000000000ULL)  |
		((((uint64)inValue)<< 8) & 0x000000FF00000000ULL)  |
		((((uint64)inValue)>> 8) & 0x00000000FF000000ULL)  |
		((((uint64)inValue)>>24) & 0x0000000000FF0000ULL)  |
		((((uint64)inValue)>>40) & 0x000000000000FF00ULL)  |
		((((uint64)inValue)>>56) & 0x00000000000000FFULL));
}

inline
int32 byte_swapper::swap(int32 inValue)
{
	return static_cast<int32>(
			((((uint32)inValue)<<24) & 0xFF000000)  |
			((((uint32)inValue)<< 8) & 0x00FF0000)  |
			((((uint32)inValue)>> 8) & 0x0000FF00)  |
			((((uint32)inValue)>>24) & 0x000000FF));
}

inline
uint32 byte_swapper::swap(uint32 inValue)
{
	return static_cast<uint32>(
			((((uint32)inValue)<<24) & 0xFF000000)  |
			((((uint32)inValue)<< 8) & 0x00FF0000)  |
			((((uint32)inValue)>> 8) & 0x0000FF00)  |
			((((uint32)inValue)>>24) & 0x000000FF));
}

inline
float byte_swapper::swap(float inValue)
{
	union {
		float	a;
		uint32	b;
	} v;
	
	v.a = inValue;
	
	v.b = static_cast<uint32>(
			((v.b<<24) & 0xFF000000)  |
			((v.b<< 8) & 0x00FF0000)  |
			((v.b>> 8) & 0x0000FF00)  |
			((v.b>>24) & 0x000000FF));
	
	return v.a;
}

inline
int16 byte_swapper::swap(int16 inValue)
{
	return static_cast<int16>(
			((((uint16)inValue)<< 8) & 0xFF00)  |
			((((uint16)inValue)>> 8) & 0x00FF));
}

inline
uint16 byte_swapper::swap(uint16 inValue)
{
	return static_cast<uint16>(
			((((uint16)inValue)<< 8) & 0xFF00)  |
			((((uint16)inValue)>> 8) & 0x00FF));
}

inline
int8 byte_swapper::swap(int8 inValue)
{
	return inValue;
}

inline
uint8 byte_swapper::swap(uint8 inValue)
{
	return inValue;
}

inline
bool byte_swapper::swap(bool inValue)
{
	return inValue;
}

#if P_BIGENDIAN
typedef no_swapper		net_swapper;
#else
typedef byte_swapper	net_swapper;
#endif

#endif // HBYTESWAP_H
