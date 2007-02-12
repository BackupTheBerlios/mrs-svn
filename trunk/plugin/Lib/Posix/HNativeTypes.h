/*	$Id: HNativeTypes.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Hekkelman Programmatuur b.v.
	Created Wednesday August 22 2001 14:40:51
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
 
#ifndef HNATIVETYPES_H
#define HNATIVETYPES_H

//#include <X11/Xlib.h>
//#include <X11/Xutil.h>

#include <string>

#include "HTypes.h"

/*
struct HNativeRect : public XRectangle
{
	HNativeRect()
	{
	}
	HNativeRect(short inLeft, short inTop, short inRight, short inBottom)
	{
		x = inLeft;
		y = inTop;
		width = static_cast<unsigned short>(inRight - inLeft);
		height = static_cast<unsigned short>(inBottom - inTop);
	}
	HNativeRect(const HRect& inRect)
	{
		x = static_cast<short>(inRect.left);
		y = static_cast<short>(inRect.top);
		width = static_cast<unsigned short>(inRect.GetWidth());
		height = static_cast<unsigned short>(inRect.GetHeight());
	}
	HNativeRect(XRectangle& inRect)
	{
		x = inRect.x;
		y = inRect.y;
		width = inRect.width;
		height = inRect.height;
	}

	operator XRectangle*()	{ return this; }
	operator HRect()		{ return HRect(x, y, x + width, y + height); }
};

struct HNativePoint : public XPoint
{
	HNativePoint(HPoint inPoint)
	{
		x = static_cast<short>(inPoint.x);
		y = static_cast<short>(inPoint.y);
	}
};
*/

/*
struct HStr255
{
	HStr255()
		{ fText[0] = 0; }
	
	HStr255(const char* inText);
	void operator += (char inChar);
	void operator += (int inNr);
	void operator += (const char* inText);
	void operator += (const unsigned char* inText);
	
	HStr255& operator= (const char* inText);
	
	operator StringPtr()	{ return fText; }
	
	Str255	fText;
};
*/
/*
struct HXText : public XTextProperty
{
	HXText(const char* inText);
	
//	operator XTextProperty*		{ return this; }
};
*/

//struct HNativeColor : public RGBColor
//{
//	HNativeColor() {}
//	HNativeColor(HColor inColor)
//	{
//		red = inColor.red * 0x0101;
//		green = inColor.green * 0x0101;
//		blue = inColor.blue * 0x0101;
//	}
//	HNativeColor(const RGBColor& inColor)
//	{
//		red = inColor.red;
//		green = inColor.green;
//		blue = inColor.blue;
//	}
//	
//	operator RGBColor*()	{ return this; }
//	operator HColor()		{ return HColor(red >> 8, green >> 8, blue >> 8, 0); }
//};

struct HFileSpec
{
	enum { valid = false };
	
				HFileSpec();
				HFileSpec(const HFileSpec& inParent, const char* inRelativePath);

	HFileSpec	GetParent() const;
	bool		IsValid() const							{ return false; }
	bool		IsDirectory() const						{ return false; }
	
	HErrorCode	SetPath(const std::string& inPath);
	void		GetPath(std::string& outPath) const;
	void		GetNativePath(std::string& outPath) const;
	void		SetNativePath(std::string inPath);
	
	bool		operator==(const HFileSpec& /*inOther*/)	{ return false; }
	bool		operator!=(const HFileSpec& /*inOther*/)	{ return true; }
};

#endif // HNATIVETYPES_H
