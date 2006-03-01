/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Saturday January 26 2002 10:53:55
*/

#include "HLib.h"

#include "HGlobals.h"
#include "HWinUtils.h"
#include "HFile.h"
#include "HUtils.h"
#include "HDefines.h"

#include <windows.h>
#include <ShFolder.h>
#include <shlobj.h>
#include "HWinApiWrappers.h"

void InitHGlobals()
{
	/*	
		HasUnicode will initialize the gOSVersion the first time its called.
		Therefore call it here while we are not interested in the result :)
	*/
	HasUnicode ();

	/*
		No UID and GID 
	*/
	gUid = 0;
	gGid = 0;

	/* 
		Set the gTempUrl
		use only ANSI encoded paths here
	*/
	HFileSpec spec;
	HGetTempPath(spec);
	gTempURL.SetSpecifier(spec);
	
	/* 
		Set the gAppUrl
	*/

	::HGetModuleFile (NULL, spec);
	gAppURL.SetSpecifier (spec);
	gAppURL = gAppURL.GetParent();

	::HSHGetFolderPath(nil, CSIDL_PERSONAL | CSIDL_FLAG_CREATE,
		nil, SHGFP_TYPE_CURRENT, spec);
	if (spec.IsDirectory())
	{
		gUserURL = HUrl(spec).GetChild(kAppName);
		if (not HFile::Exists(gUserURL))
			HFile::MkDir(gUserURL);
	}

	assert(gTempURL.IsDirectory());

	::HSHGetFolderPath(nil, CSIDL_APPDATA | CSIDL_FLAG_CREATE,
		nil, SHGFP_TYPE_CURRENT, spec);
	if (spec.IsDirectory())
	{
		gPrefsURL = HUrl(spec).GetChild(kAppName);
		if (not HFile::Exists(gPrefsURL))
			HFile::MkDir(gPrefsURL);
	}
	
	if (not HFile::Exists(gPrefsURL))
		gPrefsURL = gUserURL.GetChild("Settings");
	
	//HColor gray = HFromWinColor(COLORREF(::GetSysColor(COLOR_BTNFACE)));

	//gColor[kActiveDocBackColor] = gray;
	//gColor[kInactiveDocBackColor] = gray;
	//gColor[kActiveDlogBackColor] = gray;
	//gColor[kInactiveDlogBackColor] = gray;
	//
	//gColor[kActiveLineColor] = gray.Disable(kBlack);
	//gColor[kInactiveLineColor] = gray.Darker();
	//
	//gColor[kOSSelectionColor] = HFromWinColor(COLORREF(::GetSysColor(COLOR_HIGHLIGHT)));
	//gColor[kOSSelectionTextColor] = HFromWinColor(COLORREF(::GetSysColor(COLOR_HIGHLIGHTTEXT)));
	//gColor[kOSInactiveSelectionColor] = gray;
	//gColor[kOSInactiveSelectionTextColor] = kBlack;
}


