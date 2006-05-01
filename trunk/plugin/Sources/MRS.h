/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Saturday December 07 2002 12:51:07
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
 
#ifndef MRS_H
#define MRS_H

#include "HLib.h"
#include <limits>

//#if P_WIN
//#include "DebugNew.h"
//#endif

#if P_BIGENDIAN
#define FOUR_CHAR_INLINE(x)	x
#else
#define FOUR_CHAR_INLINE(x)     \
			((((x)<<24) & 0xFF000000UL)|\
			 (((x)<<8 ) & 0x00FF0000UL)|\
			 (((x)>>8 ) & 0x0000FF00UL)|\
			 (((x)>>24) & 0x000000FFUL))
#endif

// some globals, settable from the outside

#include <string>

enum CIndexKind
{
	kTextIndex		= FOUR_CHAR_INLINE('text'),
	kWeightedIndex	= FOUR_CHAR_INLINE('wtxt'),

			// unique id, each key points to only one doc
	kValueIndex		= FOUR_CHAR_INLINE('valu'),
	kDateIndex		= FOUR_CHAR_INLINE('date'),
	
			// index containing ascii representation of an integer
	kNumberIndex	= FOUR_CHAR_INLINE('nmbr')
};

const uint32
	kWeightBitCount = 6,
	kMaxWeight = ((1 << kWeightBitCount) - 1);

const uint32
	kInvalidDocID = std::numeric_limits<uint32>::max();

enum CQueryOperator
{
	kOpContains,		// default, the google like operator
	kOpEquals,			// exact match, works only with Number, Date and Value indexes
	kOpLessThan,
	kOpLessEqual,
	kOpEqual,
	kOpGreaterEqual,
	kOpGreaterThan
};

extern int VERBOSE;
extern unsigned int THREADS;
extern const char* COMPRESSION;
extern int COMPRESSION_LEVEL;
extern const char* COMPRESSION_DICTIONARY;
extern const char* kFileExtension;

#endif // MRS_H
