/*	$Id: HByteSwap.h,v 1.25 2005/08/22 12:38:04 maarten Exp $
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
	static T	swap(const T inValue)		{ return inValue; }
};

// a class that swaps
struct byte_swapper
{
	static bool		swap(bool inValue);
	static int8		swap(int8 inValue);
	static int16	swap(int16 inValue);
	static int32	swap(int32 inValue);
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
int32 byte_swapper::swap(const int32 inValue)
{
	return static_cast<int32>(
			((((uint32)inValue)<<24) & 0xFF000000)  |
			((((uint32)inValue)<< 8) & 0x00FF0000)  |
			((((uint32)inValue)>> 8) & 0x0000FF00)  |
			((((uint32)inValue)>>24) & 0x000000FF));
}

inline
uint32 byte_swapper::swap(const uint32 inValue)
{
	return static_cast<uint32>(
			((((uint32)inValue)<<24) & 0xFF000000)  |
			((((uint32)inValue)<< 8) & 0x00FF0000)  |
			((((uint32)inValue)>> 8) & 0x0000FF00)  |
			((((uint32)inValue)>>24) & 0x000000FF));
}

inline
int16 byte_swapper::swap(const int16 inValue)
{
	return static_cast<int16>(
			((((uint16)inValue)<< 8) & 0xFF00)  |
			((((uint16)inValue)>> 8) & 0x00FF));
}

inline
uint16 byte_swapper::swap(const uint16 inValue)
{
	return static_cast<uint16>(
			((((uint16)inValue)<< 8) & 0xFF00)  |
			((((uint16)inValue)>> 8) & 0x00FF));
}

inline
int8 byte_swapper::swap(const int8 inValue)
{
	return inValue;
}

inline
uint8 byte_swapper::swap(const uint8 inValue)
{
	return inValue;
}

inline
bool byte_swapper::swap(const bool inValue)
{
	return inValue;
}

#if P_BIGENDIAN
typedef no_swapper		net_swapper;
#else
typedef byte_swapper	net_swapper;
#endif
//
//
//template <class T>
//inline
//T bswap(T a);
//
//#if P_MAC
//
//#if P_GNU
//
//#include <Endian.h>
//
//#define	REVERSE_FETCH16(a)	Endian16_Swap(a)
//#define	REVERSE_FETCH32(a)	Endian32_Swap(a)
//#else
//#include "fastbyteswap.h"
//#endif
//
//template <>
//inline
//short bswap(short a)
//{
//	return static_cast<short>(REVERSE_FETCH16(&a));
//}
//
//template <>
//inline
//unsigned short bswap(unsigned short a)
//{
//	return static_cast<unsigned short>(REVERSE_FETCH16(&a));
//}
//
//template <>
//inline
//long bswap(long a)
//{
//	return static_cast<long>(REVERSE_FETCH32(&a));
//}
//
//template <>
//inline
//unsigned long bswap(unsigned long a)
//{
//	return static_cast<unsigned long>(REVERSE_FETCH32(&a));
//}
//
//#elif HAS_ENDIAN
//
//template <>
//inline
//short bswap(short a)
//{
//	return (short) ((((int)a & 0xFF00) >> 8) | (((int)a & 0x00FF) << 8));
//}
//
//template <>
//inline
//unsigned short bswap(unsigned short a)
//{
//	return (unsigned short) bswap ((short) a);
//}
//
//template <>
//inline
//long bswap(long a)
//{
//	return (long) ((((unsigned int)a & 0xFF000000UL) >> 24UL) | (((unsigned int)a & 0x00FF0000UL) >> 8UL)
//				| (((unsigned int)a & 0x0000FF00UL) << 8UL) | (((unsigned int)a & 0x000000FFUL) << 24UL));
//}
//
//template <>
//inline
//unsigned long bswap(unsigned long a)
//{
//	return (unsigned long) bswap ((long) a);
//}
//
//#elif P_WIN
//
//template <>
//inline
//short bswap(short a)
//{
//	return (short) ((((int)a & 0xFF00) >> 8) | (((int)a & 0x00FF) << 8));
//}
//
//template <>
//inline
//unsigned short bswap(unsigned short a)
//{
//	return (unsigned short) bswap ((short) a);
//}
//
//template <>
//inline
//long bswap(long a)
//{
//	return (long) ((((unsigned int)a & 0xFF000000UL) >> 24UL) | (((unsigned int)a & 0x00FF0000UL) >> 8UL)
//				| (((unsigned int)a & 0x0000FF00UL) << 8UL) | (((unsigned int)a & 0x000000FFUL) << 24UL));
//}
//
//template <>
//inline
//unsigned long bswap(unsigned long a)
//{
//	return (unsigned long) bswap ((long) a);
//}
//
//#else
//#include <byteswap.h>
//
//template <>
//inline
//short bswap(short a)
//{
//	return __bswap_16(a);
//}
//
//template <>
//inline
//unsigned short bswap(unsigned short a)
//{
//	return __bswap_16(a);
//}
//
//template <>
//inline
//long bswap(long a)
//{
//	return __bswap_32(a);
//}
//
//template <>
//inline
//unsigned long bswap(unsigned long a)
//{
//	return __bswap_32(a);
//}
//
//#endif

#endif // HBYTESWAP_H
