/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created By Bas Vodde Saturday January 12 2002 09:23:52
*/

#ifndef HWINUTILS_H
#define HWINUTILS_H

#include "HStlString.h"

/*
	Should be in HWinGlobals.h but would be the only one 
*/
extern long  gOSVersionMinor;

#define HasUnicode()	true

//bool InitHasUnicode ();
//
//inline bool 
//HasUnicode ()
//{
////#if P_DEBUG
////	static bool gUnicode = InitHasUnicode ();
////	return gUnicode;
////#else
//	return true;
////#endif
//}

void UnixToWinPath(std::string& ioPath);
void WinToUnixPath(std::string& ioPath);

#ifndef MINI_H_LIB
void GetModifierState(unsigned long& outModifiers, bool inAsync);
void RegisterFileExtension(const char* inExt, const char* inDesc);
void RestoreFileExtension(const char* inExt);
bool NextAssoc(unsigned long& ioCookie, std::string& outExt, std::string& outDesc);

class StSaveDC
{
  public:
		  	StSaveDC();
			StSaveDC(void* inDC);
			~StSaveDC();
	
	void*	GetDC() const	{ return fDC; }
  private:
  	void*	fDC;
  	int		fState;
};
#endif

#endif
