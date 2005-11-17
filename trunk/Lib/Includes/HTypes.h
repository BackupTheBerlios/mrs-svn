/*	$Id: HTypes.h,v 1.45 2005/08/22 12:38:05 maarten Exp $
	
	Copyright Hekkelman Programmatuur b.v.
	Maarten Hekkelman
	
	Created: 10 August, 1999 20:25:32
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
 
#ifndef HTYPES_H
#define HTYPES_H

#include "HDefines.h"
#include "HStdCStdlib.h"

typedef unsigned long	HUnicode;
typedef long			HErrorCode;

// Encodings

enum HEncoding {
	enc_Unknown = -1,
	enc_UTF8 = 0,
	enc_UTF16,
	enc_UTF16LE,
	enc_MacRoman,
	enc_win1252,	// this is Windows ANSI
	enc_iso1,
	enc_iso2,
	enc_iso3,
	enc_iso4,
	enc_iso5,
	enc_iso6,
	enc_iso7,
	enc_iso8,
	enc_iso9,
	enc_iso10,
	enc_iso13,
	enc_iso14,
	enc_iso15,
//	enc_ShiftJIS,
//	enc_Big5,
//	enc_UTF7,
//	enc_iso16,
	enc_Count
};

#if P_MAC
#define enc_Native	enc_MacRoman
#elif P_UNIX
#define enc_Native	enc_iso1
#elif P_WIN
#define enc_Native enc_win1252
#else
#error
#endif

// Types for HNode (and HHandler)

typedef unsigned long HNodeID;

enum HTriState
{
	kTriStateOff,
	kTriStateOn,
	kTriStateLatent
};

enum {	// inline text input constants
	kHActiveInputArea			= 0,	/* entire active input area */
	kHCaretPosition				= 1,	/* specify caret position */
	kHRawText 					= 2,	/* specify range of raw text */
	kHSelectedRawText 			= 3,	/* specify range of selected raw text */
	kHConvertedText				= 4,	/* specify range of converted text */
	kHSelectedConvertedText		= 5,	/* specify range of selected converted text */
	kHBlockFillText				= 6,	/* Block Fill hilite style */
	kHOutlineText 				= 7,	/* Outline hilite style */
	kHSelectedText				= 8		/* Selected hilite style */
};

enum HTextInputRegionClass {
	kHTSMOutsideOfBody			 = 1,
	kHTSMInsideOfBody 			 = 2,
	kHTSMInsideOfActiveInputArea = 3
};

// HEditText types
enum HAlignment {
	hAlignLeft,
	hAlignCenter
};

enum HFilterResult {
	hDefault,		// handle in the default way
	hIgnore,		// ignore this character
	hReject,		// reject this character
	hPassUp			// pass to next handler
};

// HHandler types
struct HTextInputAreaInfo
{
	long				fOffset[9];
	long				fLength[9];
};

// HTSMDoc is a class that helps controlling inline input of asian scripts
class HHandler;
class HWindow;
class HTSMDoc
{
  public:
	HTSMDoc(HWindow* inWindow, HHandler* inHandler);
	~HTSMDoc();
	
	void Activate();
	void Deactivate();
	void Fix();
  private:
	struct HTSMDocImp*	fImpl;
};

// HFont
enum HFontFace
{
	kFontNormal = 0,
	kFontBold = 1,
	kFontItalic = 2,
	kFontUnderline = 4
};

struct HFontInfo
{
	int ascent;
	int descent;
	int leading;
};

#if P_DEST_OS != P_MAC_OS
typedef unsigned char	Str255[256];
typedef unsigned char	Str32[32];
typedef unsigned char*	StringPtr;
typedef const unsigned char*	ConstStringPtr;
#endif

struct HColor
{
	unsigned char alpha, red, green, blue;
	
	HColor()
		: alpha(0xff)
		, red(0)
		, green(0)
		, blue(0)
		{}
	HColor(const HColor& c)
	{
		*reinterpret_cast<long*>(this) = *reinterpret_cast<const long*>(&c);
	}
	HColor(unsigned char R, unsigned char G, unsigned char B, unsigned char A)
		: alpha (A)
		, red (R)
		, green (G)
		, blue (B) {}
	explicit HColor(unsigned long inColor);
	
	HColor	Distinct(HColor inBackColor) const;
	HColor	Disable(HColor inBackColor) const;
	HColor	Darker() const;
	HColor	Lighter() const;
	
	void	Invert ();

	HColor&	operator= (unsigned long inValue);
	unsigned long
			GetValue() const;
	
	bool operator== (HColor inColor) const
	{
		return	red == inColor.red &&
				green == inColor.green &&
				blue == inColor.blue;
//				 &&	alpha == inColor.alpha
	}
	bool operator != (HColor inColor) const
	{
		return	! (operator == (inColor));
	}
};

const HColor
	kWhite(255, 255, 255, 255),
	kBlack(0, 0, 0, 255);

struct HPoint
{
	long y, x;
	
	HPoint()								{ x = 0; y = 0; }
	HPoint(const HPoint& inOther)			{ x = inOther.x; y = inOther.y; }
	HPoint (long X, long Y)					{ x = X; y = Y; }
	
	void Set(long X, long Y)				{ x = X; y = Y; }
	
	void OffsetBy(long inH, long inV)		{ x += inH; y += inV; }
	
	bool operator==(const HPoint& inOther) const
											{ return x == inOther.x and y == inOther.y; }
	bool operator!=(const HPoint& inOther) const
											{ return x != inOther.x or y != inOther.y; }
};

struct HNativeRect;

struct HRect
{
	long top, left, bottom, right;
	
	HRect()										{ left = 0; top = 0; right = 0; bottom = 0; }
	HRect(long l, long t, long r, long b)		{ left = l; top = t; right = r; bottom = b; }
//	HRect(const Rect& r)						{ left = r.left; top = r.top; right = r.right; bottom = r.bottom; }
	HRect(const HNativeRect& inRect);
	
	HRect& operator=(const HNativeRect& inRect);
	
	void Set(long l, long t, long r, long b)	{ left = l; top = t; right = r; bottom = b; }
	
	int GetWidth() const						{ return right - left; }
	int GetHeight() const						{ return bottom - top; }

	bool Intersects(const HRect& r) const		{ return (r.right > left && r.bottom > top && r.left < right && r.top < bottom); } 
	bool Contains(const HPoint& p) const		{ return p.x >= left && p.x <= right && p.y >= top && p.y <= bottom; }
	void OffsetBy(int x, int y)					{ left += x; right += x; top += y; bottom += y; }
	void OffsetTo(int x, int y)					{ right += x - left; left = x; bottom += y - top; top = y; }
	void InsetBy(int h, int v)					{ left += h, right -= h, top += v, bottom -= v; }
	bool IsValid() const						{ return left < right && bottom > top; }
	bool IsEmpty() const						{ return left >= right || bottom <= top; }

	bool operator==(const HRect& inOther) const;
	bool operator!=(const HRect& inOther) const;
	
	HRect& operator &= (HRect inOther);
	HRect& operator |= (HRect inOther);
	
	HPoint LeftTop () const						{ return HPoint (left, top); }
	HPoint LeftBottom () const					{ return HPoint (left, bottom); }
	HPoint RightTop () const					{ return HPoint (right, top); }
	HPoint RightBottom () const					{ return HPoint (right, bottom); }
};

inline HRect operator& (const HRect& a, const HRect& b)
{
	HRect r;
	r.left = a.left > b.left ? a.left : b.left;
	r.top = a.top > b.top ? a.top : b.top;
	r.right = a.right < b.right ? a.right : b.right;
	r.bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
	return r;
}

//inline HRect operator| (const HRect& a, const HRect& b)
//{
//	HRect r;
//	r.left = a.left > b.left ? a.left : b.left;
//	r.top = a.top > b.top ? a.top : b.top;
//	r.right = a.right < b.right ? a.right : b.right;
//	r.bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
//	return r;
//} // operator&

inline HRect& HRect::operator &= (HRect inOther)
{
	if (inOther.left > left)		left = inOther.left;
	if (inOther.top > top)			top = inOther.top;
	if (inOther.right < right)		right = inOther.right;
	if (inOther.bottom < bottom)	bottom = inOther.bottom;
	return *this;
}

inline HRect& HRect::operator |= (HRect inOther)
{
	if (inOther.left < left)		left = inOther.left;
	if (inOther.top < top)			top = inOther.top;
	if (inOther.right > right)		right = inOther.right;
	if (inOther.bottom > bottom)	bottom = inOther.bottom;
	return *this;
}

inline bool HRect::operator==(const HRect& inOther) const
{
	return
		left == inOther.left &&
		top == inOther.top &&
		right == inOther.right &&
		bottom == inOther.bottom;
}

inline bool HRect::operator!=(const HRect& inOther) const
{
	return
		left != inOther.left ||
		top != inOther.top ||
		right != inOther.right ||
		bottom != inOther.bottom;
}

//typedef FSSpec HFileSpec;
typedef char HSearchString[256];
typedef char HFileName[NAME_MAX];

enum HScrollKind {
	kHScrollLineUp,
	kHScrollLineDown,
	kHScrollPageUp,
	kHScrollPageDown,
	kHScrollToHere
};

class StPortAndClipSaver
{
  public:
	StPortAndClipSaver();
	StPortAndClipSaver(HRect inClip);
	~StPortAndClipSaver();
	void Restore();
  private:
	struct StPortAndClipSaverImp*	fImpl;
};

#if P_DEST_OS != P_MAC_OS
inline StPortAndClipSaver::StPortAndClipSaver()
{
}

inline StPortAndClipSaver::StPortAndClipSaver(HRect /* inClip */)
{
}

inline StPortAndClipSaver::~StPortAndClipSaver()
{
}

inline void StPortAndClipSaver::Restore()
{
}

namespace HBSD {	// move to HTypes.h someday
#if P_WIN
typedef unsigned int socket_t;
#else
typedef int socket_t;
#endif
extern const socket_t kInvalidSocket;
}

#endif

#endif
