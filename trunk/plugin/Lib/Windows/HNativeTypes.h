/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde on Friday January 11 2001 14:40:51
*/

#ifndef HNATIVETYPES_H
#define HNATIVETYPES_H

/*
	Native rectangle and point implementation. The implementations are
	derived from the RECT or POINT struct. Not really needed IMHO but keep
	the tradition of the rest of the library.
*/

#include "HTypes.h"
#include "HStlString.h"
#include "HEncoding.h"

/*	
	We REALLY DO NOT want to include windows.h. The only thing which does not
	allow to just include windef is the _X86_. So we define it ourself.
*/
#if P_VISUAL_CPP
	#pragma warning (push)
	#pragma warning (disable: 4201)
#endif

#ifndef _X86_
#define _X86_
#endif

#define NOMINMAX
#include <windef.h>

#if P_VISUAL_CPP
	#pragma warning (pop)
#endif

//struct HNativeRect : public tagRECT
//{
//	HNativeRect()
//	{
//	}
//	HNativeRect(short inLeft, short inTop, short inRight, short inBottom)
//	{
//		left = inLeft;
//		top = inTop;
//		right = inRight;
//		bottom = inBottom;
//	}
//	HNativeRect(const HRect& inRect)
//	{
//		left = inRect.left;
//		top = inRect.top;
//		right = inRect.right;
//		bottom = inRect.bottom;
//	}
//	HNativeRect(RECT& inRect)
//	{
//		left = inRect.left;
//		top = inRect.top;
//		right = inRect.right;
//		bottom = inRect.bottom;
//	}
//	operator RECT*()		{ return this; }
//	operator HRect()		{ return HRect(left, top, right, bottom); }
//};
//
//inline
//HRect::HRect(const HNativeRect& inRect)
//{
//	top = inRect.top;
//	left = inRect.left;
//	bottom = inRect.bottom;
//	right = inRect.right;
//}
//	
//inline
//HRect& 
//HRect::operator=(const HNativeRect& inRect)
//{
//	top = inRect.top;
//	left = inRect.left;
//	bottom = inRect.bottom;
//	right = inRect.right;
//	return *this;
//}
//
//struct HNativePoint : public tagPOINT
//{
//	HNativePoint(HPoint inPoint)
//	{
//		x = inPoint.x;
//		y = inPoint.y;
//	}
//};
//
//struct HStr255
//{
//	HStr255()
//		{ fText[0] = 0; }
//	
//	HStr255(const char* inText);
//	void operator += (char inChar);
//	void operator += (int inNr);
//	void operator += (const char* inText);
//	void operator += (const unsigned char* inText);
//	
//	HStr255& operator= (const char* inText);
//	
//	operator StringPtr()	{ return fText; }
//	
//	Str255	fText;
//};

/*
	Since windows paths are not the same as posix paths _and_ because
	windows paths need special processing (DOS vs NT paths) there now is
	a HFileSpec for Windows. The member data for this HFileSpec is a
	natively encoded path.
*/
struct HFileSpec
{
#if P_DEBUG
	static bool valid;
#else
	enum { valid = true };
#endif
				HFileSpec();
	explicit	HFileSpec(std::string inNativePath);
	explicit	HFileSpec(const wchar_t* inNativePath);
				HFileSpec(const HFileSpec& inParent, const char* inRelativePath);
				HFileSpec(const HFileSpec& inParent, const wchar_t* inRelativePath);

	HFileSpec	GetParent() const;
	bool		IsValid() const;
	bool		IsDirectory() const;
	
	HErrorCode	SetPath(std::string inPath);
	void		GetPath(std::string& outPath) const;
	void		GetNativePath(std::string& outPath) const;
	void		SetNativePath(std::string inPath);
	
	bool		operator==(const HFileSpec& inOther);
	bool		operator!=(const HFileSpec& inOther);

	std::wstring	GetWCharPath() const;
	std::string		GetCharPath() const;

  private:
	std::string	fNativePath;
};

class HWTStr
{
  public:
					HWTStr(const char* inUTF8, unsigned long inLength = 0);
					~HWTStr();

	const wchar_t*	get_wchar();
	char*			get_char();

	operator		const wchar_t*()	{ return get_wchar(); }	
	operator		char*()				{ return get_char(); }
	
  private:
	const char*		fUTF8;
	unsigned long	fLength;
	UTF16String		fBuffer;
};

//class HWFStr
//{
//  public:
//					HWFStr(char* inBuffer, unsigned long inBufferSize);
//					HWFStr(char* inBuffer, unsigned long* inBufferSize);
//					~HWFStr();
//
//	wchar_t*		get_wchar();
//	char*			get_char();
//
//	operator		wchar_t*()		{ return get_wchar(); }	
//	operator		char*()			{ return get_char(); }
//
//  private:
//	char*			fBuffer;
//	char*			fPrivateBuffer;
//	unsigned long	fLength;
//	bool			fUnicode;
//};

//inline
//COLORREF HToWinColor(HColor inColor)
//{
//	return static_cast<COLORREF>((inColor.blue << 16) | (inColor.green << 8) | inColor.red);
//}
//
//inline
//HColor HFromWinColor(COLORREF inColor)
//{
//	return HColor(
//		(unsigned char) ((inColor >> 0) & 0x000000FF),
//		(unsigned char) ((inColor >> 8) & 0x000000FF),
//		(unsigned char) ((inColor >> 16) & 0x000000FF),
//		0x0FF);
//}

#endif // HNATIVETYPES_H
