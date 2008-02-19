/*	$Id: HWinUtils.h 30 2006-04-30 17:36:03Z maarten $
	Copyright Hekkelman Programmatuur b.v.
	Created By Bas Vodde Saturday January 12 2002 09:23:52
*/

#ifndef HWINUTILS_H
#define HWINUTILS_H

#include <string>

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

#endif
