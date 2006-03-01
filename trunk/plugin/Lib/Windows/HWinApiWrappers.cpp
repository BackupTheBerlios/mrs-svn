/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde on Thursday January 31 2002 14:40:51
*/


#include "HLib.h"

#include "HStdCString.h"

#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

#include "HNativeTypes.h"
#include "HWinUtils.h"
#ifndef MINI_H_LIB
#include "HApplication.h"
#include "HApplicationImp.h"
#include "HEncoder.h"
#include "HPreferences.h"
#endif
#include "HUtils.h"
#include "HDefines.h"

#include "HWinApiWrappers.h"

///* Module functions */
//HMODULE 
//HGetModuleHandle (const char* lpModuleName)
//{
//	HMODULE lResult = NULL;
//	if (HasUnicode ()) {
//		lResult = ::GetModuleHandleW(HWTStr(lpModuleName));
//	}
//	else {
//		lResult = ::GetModuleHandleA(HWTStr(lpModuleName));
//	}
//	return lResult;
//}
//
//void HGetCommandLine(char*& outBuffer)
//{
//	outBuffer = nil;
//	if (HasUnicode())
//	{
//		wchar_t* cmdLine = ::GetCommandLineW();
//		if (*cmdLine == 0)
//		{
//			cmdLine = new wchar_t[NAME_MAX];
//			::GetModuleFileNameW(nil, cmdLine, NAME_MAX);
//		}
//		
//		unsigned long s1 = 2UL * (::lstrlenW(cmdLine) + 1);
//		unsigned long s2 = 2 * s1;
//		outBuffer = new char[s2];
//		HEncoder::FetchEncoder(enc_UTF16LE)->
//			EncodeToUTF8((char*)cmdLine, s1, outBuffer, s2);
//	}
//	else
//	{
//		char* cmdLine = ::GetCommandLineA();
//		if (*cmdLine == 0)
//		{
//			cmdLine = new char[NAME_MAX];
//			::GetModuleFileNameA(nil, cmdLine, NAME_MAX);
//		}
//		
//		unsigned long s1 = ::lstrlenA(cmdLine) + 1UL;
//		unsigned long s2 = 2 * s1;
//		outBuffer = new char[s2];
//		HEncoder::FetchEncoder(enc_Native)->
//			EncodeToUTF8(cmdLine, s1, outBuffer, s2);
//	}
//}
//
std::string HGetCurrentDirectory()
{
	//char* result = nil;
	//if (HasUnicode())
	//{
		DWORD len = ::GetCurrentDirectoryW(0, nil);
		HAutoBuf<wchar_t> path(new wchar_t[len]);
		
		len = ::GetCurrentDirectoryW(len, path.get());
		return Convert(UTF16String(path.get(), len));

	//}
	//else
	//{
	//	DWORD len = ::GetCurrentDirectoryW(0, nil);
	//	HAutoBuf<char> path(new char[len]);
	//	
	//	len = ::GetCurrentDirectoryA(len, path.get());
	//	
	//	unsigned long s1 = len + 1UL;
	//	unsigned long s2 = 2 * s1;
	//	result = new char[s2];
	//	HEncoder::FetchEncoder(enc_Native)->
	//		EncodeToUTF8(path.get(), s1, result, s2);
	//}
	//return result;
}

DWORD HGetTempFileName(const HFileSpec& inTempDir, const char* inBasename,
	HFileSpec& outTempFile)
{
	DWORD result;
	HWTStr name(inBasename);

	if (HasUnicode())
	{
		std::wstring dir(inTempDir.GetWCharPath());
		HAutoBuf<wchar_t> file(new wchar_t[PATH_MAX]);
		result = ::GetTempFileNameW(dir.c_str(), name, 0, file.get());
		outTempFile = HFileSpec(file.get());
	}
	else
	{
		std::string dir(inTempDir.GetCharPath());
		HAutoBuf<char> file(new char[PATH_MAX]);
		result = ::GetTempFileNameA(dir.c_str(), name, 0, file.get());
		outTempFile = HFileSpec(file.get());
	}
	return result;
}

void HGetTempPath(HFileSpec& outTempDir)
{
	if (HasUnicode())
	{
		HAutoBuf<wchar_t> path(new wchar_t[PATH_MAX]);
		::GetTempPathW(PATH_MAX, path.get());
		outTempDir = HFileSpec(path.get());
	}
	else
	{
		HAutoBuf<char> path(new char[PATH_MAX]);
		::GetTempPathA(PATH_MAX, path.get());
		outTempDir = HFileSpec(path.get());
	}
}

void HGetModuleFile(HMODULE inModule, HFileSpec& outSpec)
{
	if (HasUnicode())
	{
		HAutoBuf<wchar_t> path(new wchar_t[PATH_MAX]);
		::GetModuleFileNameW(inModule, path.get(), PATH_MAX);
		outSpec = HFileSpec(path.get());
	}
	else
	{
		HAutoBuf<char> path(new char[PATH_MAX]);
		::GetModuleFileNameA(inModule, path.get(), PATH_MAX);
		outSpec = HFileSpec(path.get());
	}
}

void	HSHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, HFileSpec& outSpec)
{
	if (HasUnicode())
	{
		HAutoBuf<wchar_t> path(new wchar_t[PATH_MAX]);
		::SHGetFolderPathW(hwnd, csidl, hToken, dwFlags, path.get());
		outSpec = HFileSpec(path.get());
	}
	else
	{
		HAutoBuf<char> path(new char[PATH_MAX]);
		::SHGetFolderPathA(hwnd, csidl, hToken, dwFlags, path.get());
		outSpec = HFileSpec(path.get());
	}
}

/* Message loop functions */
BOOL 
HGetMessage (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	BOOL result;
	if (HasUnicode ()) {
		result = ::GetMessageW (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	else {
		result = ::GetMessageA (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
	}
	return result;
}

BOOL	HPeekMessage (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	BOOL result;
	if (HasUnicode ()) {
		result = ::PeekMessageW (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	}
	else {
		result = ::PeekMessageA (lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
	}
	return result;
}

BOOL HIsDialogMessage (HWND hWnd, LPMSG lpMsg)
{
	BOOL result;
	if (HasUnicode ()) {
		result = ::IsDialogMessageW (hWnd, lpMsg);
	}
	else {
		result = ::IsDialogMessageA (hWnd, lpMsg);
	}
	return result;
}

LONG 
HDispatchMessage (CONST MSG *lpmsg)
{
	LONG result;
	if (HasUnicode ()) {
		result = ::DispatchMessageW (lpmsg);
	}
	else {
		result = ::DispatchMessageA (lpmsg);
	}
	return result;
}

BOOL 
HPostMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	BOOL lResult;
	if (HasUnicode ()) {
		lResult = ::PostMessageW (hWnd, Msg, wParam, lParam);
	}
	else {
		lResult = ::PostMessageA (hWnd, Msg, wParam, lParam);
	}
	return lResult;
}

LRESULT 
HSendMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lResult;
	if (HasUnicode ()) {
		lResult = ::SendMessageW (hWnd, Msg, wParam, lParam);
	}
	else {
		lResult = ::SendMessageA (hWnd, Msg, wParam, lParam);
	}
	return lResult;
}

/* Window proc functions */
LRESULT 
HDefWindowProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	if (HasUnicode ()) {
		result = ::DefWindowProcW (hWnd, Msg, wParam, lParam);
	}
	else {
		result = ::DefWindowProcA (hWnd, Msg, wParam, lParam);
	}
	return result;
}

LRESULT 
HDefDlgProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	if (HasUnicode ()) {
		result = ::DefDlgProcW (hWnd, Msg, wParam, lParam);
	}
	else {
		result = ::DefDlgProcA (hWnd, Msg, wParam, lParam);
	}
	return result;
}

LRESULT 
HDefFrameProc (HWND hWnd, HWND hWndMDIClient, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	if (HasUnicode ()) {
		result = ::DefFrameProcW (hWnd, hWndMDIClient, uMsg, wParam, lParam);
	}
	else {
		result = ::DefFrameProcA (hWnd, hWndMDIClient, uMsg, wParam, lParam);
	}
	return result;
}

LRESULT 
HDefMDIChildProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	if (HasUnicode ()) {
		result = ::DefMDIChildProcW (hWnd, uMsg, wParam, lParam);
	}
	else {
		result = ::DefMDIChildProcA (hWnd, uMsg, wParam, lParam);
	}
	return result;
}

LRESULT 
HCallWindowProc(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result;
	if (HasUnicode ()) {
		result = ::CallWindowProcW(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
	}
	else {
		result = ::CallWindowProcA(lpPrevWndFunc, hWnd, Msg, wParam, lParam);
	}
	return result;
}

#ifndef MINI_H_LIB
/* Window class functions */
ATOM 
HRegisterWindowClassEx (UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
							 HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground,
							 char* lpszMenuName, const char* lpszClassName,
							 HICON hIconSm)
{
	ATOM result;
	
	HWTStr className(lpszClassName);
	HWTStr menuName(lpszMenuName);
	
	if (HasUnicode ()) {
		WNDCLASSEXW lWndClass;
		::ZeroMemory (&lWndClass, sizeof (lWndClass));
		lWndClass.cbSize = sizeof (lWndClass);
		lWndClass.style = style;
		lWndClass.lpfnWndProc = lpfnWndProc;
		lWndClass.cbClsExtra = cbClsExtra;
		lWndClass.cbWndExtra = cbWndExtra;
		lWndClass.hInstance = gApp->impl ()->GetInstanceHandle ();
		lWndClass.hIcon = hIcon;
		lWndClass.hCursor = hCursor;
		lWndClass.hbrBackground = hbrBackground;
		lWndClass.lpszMenuName = menuName;
		lWndClass.lpszClassName = className;
		lWndClass.hIconSm = hIconSm;
		result = ::RegisterClassExW (&lWndClass);
	}
	else {
		WNDCLASSEXA lWndClass;
		::ZeroMemory (&lWndClass, sizeof (lWndClass));
		lWndClass.cbSize = sizeof (lWndClass);
		lWndClass.style = style;
		lWndClass.lpfnWndProc = lpfnWndProc;
		lWndClass.cbClsExtra = cbClsExtra;
		lWndClass.cbWndExtra = cbWndExtra;
		lWndClass.hInstance = gApp->impl ()->GetInstanceHandle ();
		lWndClass.hIcon = hIcon;
		lWndClass.hCursor = hCursor;
		lWndClass.hbrBackground = hbrBackground;
		lWndClass.lpszMenuName = menuName;
		lWndClass.lpszClassName = className;
		lWndClass.hIconSm = hIconSm;
		result = ::RegisterClassExA (&lWndClass);
	}
#if P_DEBUG
	if (result) {
		::SetLastError (0);
	}
#endif
	return result;

}

DWORD 
HGetClassLong (HWND hWnd, int nIndex)
{
	DWORD lResult;
	if (HasUnicode ()) {
		lResult = ::GetClassLongW (hWnd, nIndex);
	}
	else {
		lResult = GetClassLongA (hWnd, nIndex);
	}
	return lResult;
}

BOOL 
HGetClassInfoEx (const char* inClass)
{
	BOOL lResult;
	
	HWTStr lpszClass(inClass);
	if (HasUnicode ()) {
		WNDCLASSEXW lWndClass = { 0 };
		lWndClass.cbSize = sizeof(lWndClass);
		lResult = ::GetClassInfoExW (gApp->impl ()->GetInstanceHandle (), lpszClass, &lWndClass);
	}
	else {
		WNDCLASSEXA lWndClass = { 0 };
		lWndClass.cbSize = sizeof(lWndClass);
		lResult = ::GetClassInfoExA (gApp->impl ()->GetInstanceHandle (), lpszClass, &lWndClass);
	}
#if P_DEBUG
	if (!lResult && ::GetLastError () == ERROR_CLASS_DOES_NOT_EXIST) {
		::SetLastError (0); // This error is no problem since the Window class is then not registered
	}
#endif
	return lResult;
}

/* Window functions */
HWND 
HCreateWindowEx (DWORD dwExStyle, const char* lpClassName, const char* lpWindowName, DWORD dwStyle,
					int x, int y, int nWidth, int nHeight, HWND hWndParent,
					HMENU hMenu, LPVOID lpParam)
{
	HWND handle;

	if (HasUnicode ()) {
		handle = ::CreateWindowExW (dwExStyle, HWTStr(lpClassName),
			HWTStr(lpWindowName), dwStyle, x, y, nWidth, nHeight,
			hWndParent, hMenu, gApp->impl ()->GetInstanceHandle (), lpParam);

	}
	else {
		handle = ::CreateWindowExA (dwExStyle, HWTStr(lpClassName),
			HWTStr(lpWindowName), dwStyle, x, y, nWidth, nHeight,
			hWndParent, hMenu, gApp->impl ()->GetInstanceHandle (), lpParam);
	}
	return handle;
}

LONG 
HGetWindowLong (HWND hWnd, int nIndex)
{
	LONG lResult;
	if (HasUnicode ()) {
		lResult = ::GetWindowLongW (hWnd, nIndex);
	}
	else {
		lResult = GetWindowLongA (hWnd, nIndex);
	}
	return lResult;
}

LONG 
HSetWindowLong (HWND hWnd, int nIndex, LONG dwNewLong)
{
	LONG lResult;
	if (HasUnicode ()) {
		lResult = ::SetWindowLongW (hWnd, nIndex, dwNewLong);
	}
	else {
		lResult = SetWindowLongA (hWnd, nIndex, dwNewLong);
	}
	return lResult;
}

void* HGetWindowProperty(HWND hWnd, const char* inName)
{
	void* result;
	
	if (HasUnicode ()) {
		result = ::GetPropW(hWnd, HWTStr(inName));
	}
	else {
		result = ::GetPropA(hWnd, HWTStr(inName));
	}
	return result;
}

void HSetWindowProperty(HWND hWnd, const char* inName, void* inProperty)
{
	if (HasUnicode ()) {
		::SetPropW(hWnd, HWTStr(inName), (HANDLE)inProperty);
	}
	else {
		::SetPropA(hWnd, HWTStr(inName), (HANDLE)inProperty);
	}
}

void HRemoveWindowProperty(HWND hWnd, const char* inName)
{
	if (HasUnicode ()) {
		::RemovePropW(hWnd, HWTStr(inName));
	}
	else {
		::RemovePropA(hWnd, HWTStr(inName));
	}
}

int HGetWindowText (HWND hWnd, char* outString, int inStringSize)
{
	int result;
	if (HasUnicode ()) {
		result = ::GetWindowTextW (hWnd, HWFStr(outString,
			static_cast<unsigned long>(inStringSize)), inStringSize);
	}
	else {
		result = ::GetWindowTextA (hWnd, HWFStr(outString,
			static_cast<unsigned long>(inStringSize)), inStringSize);
	}
	return result;
}

BOOL 
HSetWindowText (HWND hWnd, const char* lpString)
{
	BOOL result;
	if (HasUnicode ()) {
		result = ::SetWindowTextW (hWnd, HWTStr(lpString));
	}
	else {
		result = ::SetWindowTextA (hWnd, HWTStr(lpString));
	}
	return result;
}


/* Cursor functions */
HCURSOR 
HLoadCursor (const char* lpCursorName)
{
	HCURSOR cursor;
	HWTStr name(lpCursorName);
	
	if (HasUnicode()) {
		cursor = ::LoadCursorW(NULL, name);
	}
	else {
		cursor = ::LoadCursorA(NULL, name);
	}
	return cursor;
}

HICON 
HLoadIcon (const char* lpIconName)
{
	HICON icon;
	HWTStr name(lpIconName);
	
	if (HasUnicode ()) {
		icon = ::LoadIconW(gApp->impl()->GetInstanceHandle(), name);
	}
	else {
		icon = ::LoadIconA(gApp->impl()->GetInstanceHandle(), name);
	}
	return icon;
}

/* Dialog functions */
HWND 
HCreateDialogIndirectParam (LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	HWND lWindow;
	if (HasUnicode ()) {
		lWindow = ::CreateDialogIndirectParamW (gApp->impl ()->GetInstanceHandle (), hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	else {
		lWindow = ::CreateDialogIndirectParamA (gApp->impl ()->GetInstanceHandle (), hDialogTemplate, hWndParent, lpDialogFunc, dwInitParam);
	}
	return lWindow;
}

INT_PTR 
HDialogBoxParam(HINSTANCE hInstance, const char* lpTemplateName,
		HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR result;
	if (HasUnicode())
	{
		result = ::DialogBoxParamW(hInstance, HWTStr(lpTemplateName), hWndParent,
			lpDialogFunc, dwInitParam);
	}
	else
	{
		result = ::DialogBoxParamA(hInstance, HWTStr(lpTemplateName), hWndParent,
			lpDialogFunc, dwInitParam);
	}
	return result;
}

void HSetDlgItemText(HWND inDialog, int inItem, const char* inText)
{
	HWTStr text(inText);
	if (HasUnicode())
		::SetDlgItemTextW(inDialog, inItem, text);
	else
		::SetDlgItemTextA(inDialog, inItem, text);
}

void HGetDlgItemText(HWND inDialog, int inItem, std::string& outText)
{
	if (HasUnicode())
	{
		DWORD len = 256;
		HAutoBuf<wchar_t> txt(new wchar_t[256]);
		
		len = ::GetDlgItemTextW(inDialog, inItem, txt.get(), len);
		
		outText = HEncoder::EncodeToUTF8(txt.get());
	}
	else
	{
		DWORD len = 256;
		HAutoBuf<char> txt1(new char[256]), txt2(new char[256]);
		
		len = ::GetDlgItemTextA(inDialog, inItem, txt1.get(), len);
		
		unsigned long s1 = 1, s2 = 8;
	
		HEncoder::FetchEncoder(enc_Native)->
			EncodeToUTF8(txt1.get(), s1, txt2.get(), s2);
		
		outText.assign(txt2.get(), s2);
	}
}

/* Menu functions */

BOOL
HInsertMenuItem (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii)
{
	BOOL lResult;
	
	if (HasUnicode ())
	{
		MENUITEMINFOW lItem;

		lItem.cbSize = sizeof (MENUITEMINFOW);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenW(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<wchar_t*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::InsertMenuItemW (hMenu, uItem, fByPosition, &lItem);
	}
	else
	{
		MENUITEMINFOA lItem;

		lItem.cbSize = sizeof (MENUITEMINFOA);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenA(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<char*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::InsertMenuItemA (hMenu, uItem, fByPosition, &lItem);
	}	
	return lResult;
}

BOOL 
HSetMenuItemInfo (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii)
{
	BOOL lResult;
	if (HasUnicode ())
	{
		MENUITEMINFOW lItem;

		lItem.cbSize = sizeof (MENUITEMINFOW);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenW(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<wchar_t*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::SetMenuItemInfoW (hMenu, uItem, fByPosition, &lItem);
	}
	else
	{
		MENUITEMINFOA lItem;

		lItem.cbSize = sizeof (MENUITEMINFOA);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenA(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<char*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::SetMenuItemInfoA (hMenu, uItem, fByPosition, &lItem);
	}	
	return lResult;
}

BOOL 
HGetMenuItemInfo (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii)
{
	BOOL lResult;
	if (HasUnicode ()) {
		MENUITEMINFOW lItem;

		lItem.cbSize = sizeof (MENUITEMINFOW);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenW(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<wchar_t*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::GetMenuItemInfoW (hMenu, uItem, fByPosition, &lItem);
	}
	else {
		MENUITEMINFOA lItem;

		lItem.cbSize = sizeof (MENUITEMINFOA);
		lItem.fMask = lpmii->fMask;
		lItem.fType = lpmii->fType;
		lItem.fState = lpmii->fState;
		lItem.wID = lpmii->wID;
		lItem.hSubMenu = lpmii->hSubMenu;
		lItem.hbmpChecked  = lpmii->hbmpChecked;
		lItem.hbmpUnchecked = lpmii->hbmpUnchecked;
		lItem.dwItemData = lpmii->dwItemData;
		assert (!(lItem.fMask & MIIM_TYPE) || lItem.fType == MFT_STRING || lItem.fType == MFT_SEPARATOR);
		
		HAutoPtr<HWTStr> label(nil);
		
		if (lItem.fType == MFT_STRING && (lItem.fMask & MIIM_TYPE))
		{
			label.reset(new HWTStr(lpmii->dwTypeData));
			lItem.dwTypeData = *(label.get());
			assert(lItem.dwTypeData);
			lItem.cch = static_cast<unsigned int>(::lstrlenA(lItem.dwTypeData));
		}
		else
		{
			lItem.dwTypeData = reinterpret_cast<char*> (const_cast<char*> (lpmii->dwTypeData));
			lItem.cch = lpmii->cch;
		}

		lResult = ::GetMenuItemInfoA (hMenu, uItem, fByPosition, &lItem);
	}	
	return lResult;
}

/* Font and text functions */
DWORD 
HGetCharacterPlacement (HDC inDC, const char* inText, int inLength,
		int inMaxExtent, GCP_RESULTSW& outResults, DWORD inFlags)
{
	DWORD r;

	HWTStr text(inText, static_cast<unsigned int>(inLength));

	outResults.lStructSize	= sizeof(outResults);
	outResults.nGlyphs		= static_cast<unsigned int>(inLength + 1);

	if (HasUnicode ())
		r = ::GetCharacterPlacementW(inDC, text, ::lstrlenW(text),
			inMaxExtent, &outResults, inFlags);
	else
		r = ::GetCharacterPlacementA(inDC, text, ::lstrlenA(text),
			inMaxExtent, (GCP_RESULTSA*)&outResults, inFlags);

	return r;
}

BOOL 
HExtTextOut (HDC hdc, int nXStart, int nYStart, UINT inOptions,
	CONST RECT *inClipRect, wchar_t* lpString, UINT cbString, CONST INT *lpDx)
{
	BOOL lResult;
	if (HasUnicode ()) {
		lResult = ::ExtTextOutW (hdc, nXStart, nYStart, inOptions,
			inClipRect, (wchar_t*)lpString, cbString, lpDx);
	}
	else {
		lResult = ::ExtTextOutA (hdc, nXStart, nYStart, inOptions,
			inClipRect, (char*)lpString, cbString, lpDx);
	}	
	return lResult;
}

BOOL 
HGetTextExtentPoint32 (HDC hdc, const char* inText, int inLength, HPoint& outPt)
{
	BOOL result = false;
	
	HWTStr text(inText, static_cast<unsigned int>(inLength));
	SIZE pt;
	
	if (HasUnicode ())
		result = ::GetTextExtentPoint32W(hdc, text, ::lstrlenW(text), &pt);
	else
		result = ::GetTextExtentPoint32A (hdc, text, ::lstrlenA(text), &pt);

	if (result)
	{
		outPt.x = pt.cx;
		outPt.y = pt.cy;
	}

	return result;
}

int HDrawText(HDC hdc, const char* lpString, int cbString,
	HRect& ioRect, UINT inFormat)
{
	HWTStr text(lpString, static_cast<unsigned int>(cbString));
	HNativeRect r = ioRect;
	int result;
	
	if (HasUnicode ())
		result = ::DrawTextW(hdc, text, ::lstrlenW(text), r, inFormat);
	else
		result = ::DrawTextA(hdc, text, ::lstrlenA(text), r, inFormat);

	if (result)
		ioRect = r;

	return result;
}

void 
HGetTextNameSizeAndFace(HDC hdc, char* outName, int inNameLen, int& outSize, HFontFace& outFace)
{
	HWFStr name(outName, static_cast<unsigned int>(inNameLen));
	if (HasUnicode())
	{
		TEXTMETRICW tm;

		::GetTextMetricsW(hdc, &tm);
		::GetTextFaceW(hdc, inNameLen, name);

		outSize = (-tm.tmHeight * 72) / ::GetDeviceCaps(hdc, LOGPIXELSY);
		if (tm.tmWeight >= FW_BOLD)
			outFace = kFontBold;
		else
			outFace = kFontNormal;
		if (tm.tmItalic)
			outFace = HFontFace(outFace | kFontItalic);
	}
	else
	{
		TEXTMETRICA tm;

		::GetTextMetricsA(hdc, &tm);
		::GetTextFaceA(hdc, inNameLen, name);

		outSize = (-tm.tmHeight * 72) / ::GetDeviceCaps(hdc, LOGPIXELSY);
		if (tm.tmWeight >= FW_BOLD)
			outFace = kFontBold;
		else
			outFace = kFontNormal;
		if (tm.tmItalic)
			outFace = HFontFace(outFace | kFontItalic);
	}
	
	if (outSize < 0)
		outSize = -outSize - 1;
}

#endif

/* File functions */
HANDLE 
HCreateFile(const HFileSpec& inFileName, DWORD dwDesiredAccess,
	DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	HANDLE result = nil;
	if (HasUnicode())
	{
		std::wstring path(inFileName.GetWCharPath());
		result = CreateFileW(path.c_str(),
			dwDesiredAccess, dwShareMode, lpSecurityAttributes,
			dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	else
	{
		std::string path(inFileName.GetCharPath());
		result = CreateFileA(path.c_str(),
			dwDesiredAccess, dwShareMode, lpSecurityAttributes,
			dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}
	return result;
}

void 
HCopyFile(const HFileSpec& inFile1, const HFileSpec& inFile2, bool inFailIfExists)
{
	if (HasUnicode())
	{
		std::wstring path1(inFile1.GetWCharPath());
		std::wstring path2(inFile2.GetWCharPath());
		::CopyFileW(path1.c_str(), path2.c_str(), inFailIfExists);
	}
	else
	{
		std::string path1(inFile1.GetCharPath());
		std::string path2(inFile2.GetCharPath());
		::CopyFileA(path1.c_str(), path2.c_str(), inFailIfExists);
	}
}

DWORD 
HGetFileAttributes(const HFileSpec& inFile)
{
	DWORD result;
	if (HasUnicode())
	{
		std::wstring path(inFile.GetWCharPath());
		result = ::GetFileAttributesW(path.c_str());
	}
	else
	{
		std::string path(inFile.GetCharPath());
		result = ::GetFileAttributesA(path.c_str());
	}
	return result;
}

BOOL 
HSetFileAttributes(const HFileSpec& inFile, DWORD inAttributes)
{
	BOOL result;
	if (HasUnicode())
	{
		std::wstring path(inFile.GetWCharPath());
		result = ::SetFileAttributesW(path.c_str(), inAttributes);
	}
	else
	{
		std::string path(inFile.GetCharPath());
		result = ::SetFileAttributesA(path.c_str(), inAttributes);
	}
	return result;
}

BOOL 
HDeleteFile(const HFileSpec& inFile)
{
	BOOL result;
	if (HasUnicode())
	{
		std::wstring path(inFile.GetWCharPath());
		result = ::DeleteFileW(path.c_str());
	}
	else
	{
		std::string path(inFile.GetCharPath());
		result = ::DeleteFileA(path.c_str());
	}
	return result;
}

BOOL 
HMoveFile(const HFileSpec& inFileFrom, const HFileSpec& inFileTo)
{
	BOOL result;
	if (HasUnicode())
	{
		std::wstring path1(inFileFrom.GetWCharPath());
		std::wstring path2(inFileTo.GetWCharPath());
		result = ::MoveFileW(path1.c_str(), path2.c_str());
	}
	else
	{
		std::string path1(inFileFrom.GetCharPath());
		std::string path2(inFileTo.GetCharPath());
		result = ::MoveFileA(path1.c_str(), path2.c_str());
	}
	return result;
}

BOOL 
HCreateDirectory(const HFileSpec& inDir, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
	BOOL result;
	if (HasUnicode())
	{
		std::wstring path(inDir.GetWCharPath());
		result = ::CreateDirectoryW(path.c_str(), lpSecurityAttributes);
	}
	else
	{
		std::string path(inDir.GetCharPath());
		result = ::CreateDirectoryA(path.c_str(), lpSecurityAttributes);
	}
	return result;
}

#ifndef MINI_H_LIB

BOOL 
HGetOpenFileName(DWORD inFlags, std::string& inFilter,
	const HFileSpec* inInitialDir, const char* inTitle,
	std::vector<HFileSpec>& outFiles, bool& outReadOnly)
{
	bool result = false;
	if (HasUnicode())
	{
		OPENFILENAMEW ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);
		
		HAutoBuf<wchar_t> path(new wchar_t[PATH_MAX + 1]);
		ofn.lpstrFile = path.get();
		*ofn.lpstrFile = 0;
		ofn.nMaxFile = PATH_MAX;
		
		ofn.Flags = inFlags;
		
		ofn.hwndOwner = ::GetActiveWindow ();
		ofn.hInstance = gApp->impl ()->GetInstanceHandle ();

		HWTStr wFilter(inFilter.c_str(), inFilter.length());
		
		if (inFilter.length())
		{
			ofn.lpstrFilter = wFilter;
			ofn.nFilterIndex = static_cast<unsigned long>(
				gPrefs->GetPrefInt("open filter index", 1));
		}
		
		std::wstring dir;
		if (inInitialDir)
		{
			dir = inInitialDir->GetWCharPath();
			ofn.lpstrInitialDir = dir.c_str();
		}
		
		HWTStr title(inTitle);
		ofn.lpstrTitle = title;
		
		if (::GetOpenFileNameW(&ofn))
		{
			wchar_t* p = path.get();
			
				// cut off the file name from the returned path
				// needed only when just one file was selected
			wchar_t s = p[ofn.nFileOffset];
			p[ofn.nFileOffset] = 0;
			HFileSpec dir(p);
			p[ofn.nFileOffset] = s;
			
			assert(dir.IsDirectory());
			
			wchar_t* fn = p + ofn.nFileOffset;
			while (*fn)
			{
				outFiles.push_back(HFileSpec(dir, fn));
				fn += ::lstrlenW(fn) + 1;
			}
			
			outReadOnly = (ofn.Flags & OFN_READONLY) != 0;
			gPrefs->SetPrefInt("open filter index", static_cast<int>(ofn.nFilterIndex));
			result = true;
		}
	}
	else
	{
		OPENFILENAMEA ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);
		
		HAutoBuf<char> path(new char[PATH_MAX + 1]);
		ofn.lpstrFile = path.get();
		*ofn.lpstrFile = 0;
		ofn.nMaxFile = PATH_MAX;
		
		ofn.Flags = inFlags;
		
		HWTStr filter(inFilter.c_str(), inFilter.length());
		
		ofn.hwndOwner = ::GetActiveWindow ();
		ofn.hInstance = gApp->impl ()->GetInstanceHandle ();

		if (inFilter.length())
		{
			ofn.lpstrFilter = filter;
			ofn.nFilterIndex = static_cast<unsigned long>(
				gPrefs->GetPrefInt("open filter index", 1));
		}
		
		std::string dir;
		if (inInitialDir)
		{
			dir = inInitialDir->GetCharPath();
			ofn.lpstrInitialDir = dir.c_str();
		}
		
		HWTStr title(inTitle);
		ofn.lpstrTitle = title;
		
		if (::GetOpenFileNameA(&ofn))
		{
#pragma message("decode from native!")
			char* p = path.get();
			
				// cut off the file name from the returned path
				// needed only when just one file was selected
			char s = p[ofn.nFileOffset];
			p[ofn.nFileOffset] = 0;
			HFileSpec dir(p);
			p[ofn.nFileOffset] = s;
			
			assert(dir.IsDirectory());
			
			char* fn = p + ofn.nFileOffset;
			while (*fn)
			{
				outFiles.push_back(HFileSpec(dir, fn));
				fn += ::lstrlenA(fn) + 1;
			}
			
			outReadOnly = (ofn.Flags & OFN_READONLY) != 0;
			gPrefs->SetPrefInt("open filter index", static_cast<int>(ofn.nFilterIndex));
			result = true;
		}
	}
	
	return result;
}

BOOL 
HGetSaveFileName(const char* inName, DWORD inFlags, std::string& inFilter, int& ioFilterIndex,
	const HFileSpec* inInitialDir, const char* inTitle, HFileSpec& outFile)
{
	BOOL result = false;

	if (HasUnicode())
	{
		OPENFILENAMEW ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);

		unsigned long s1, s2;
		
		HAutoBuf<wchar_t> path(new wchar_t[PATH_MAX + 1]);
		ofn.lpstrFile = path.get();

		s1 = std::strlen(inName) + 1;
		s2 = MAX_PATH;
		HEncoder::FetchEncoder(enc_UTF16LE)->
			EncodeFromUTF8(inName, s1, (char*)path.get(), s2);
		ofn.nMaxFile = PATH_MAX;

		ofn.hwndOwner = ::GetActiveWindow ();
		ofn.hInstance = gApp->impl ()->GetInstanceHandle ();
		
		ofn.Flags = inFlags;
		
		HAutoBuf<wchar_t> wFilter(nil);
		
		if (inFilter.length())
		{
			unsigned long s1 = inFilter.length();
			unsigned long s2 = 2 * s1;
			
			wFilter.reset(new wchar_t[s2]);
			HEncoder::FetchEncoder(enc_UTF16LE)->
				EncodeFromUTF8(inFilter.c_str(), s1, (char*)wFilter.get(), s2);
			
			ofn.lpstrFilter = wFilter.get();
			ofn.nFilterIndex = static_cast<unsigned long>(ioFilterIndex);
		}
		
		std::wstring dir;
		if (inInitialDir)
		{
			dir = inInitialDir->GetWCharPath();
			ofn.lpstrInitialDir = dir.c_str();
		}
		
		HWTStr title(inTitle);
		ofn.lpstrTitle = title;
		
		if (::GetSaveFileNameW(&ofn))
		{
			outFile = HFileSpec(path.get());
			ioFilterIndex = ofn.nFilterIndex;
			result = true;
		}
	}
	else
	{
		OPENFILENAMEA ofn = { 0 };
		ofn.lStructSize = sizeof(ofn);
		
		HAutoBuf<char> path(new char[PATH_MAX + 1]);
		ofn.lpstrFile = path.get();
		*ofn.lpstrFile = 0;
		ofn.nMaxFile = PATH_MAX;
		
		ofn.Flags = inFlags;
		
		HAutoBuf<char> filter(nil);
		
		if (inFilter.length())
		{
			unsigned long s1 = inFilter.length();
			unsigned long s2 = 2 * s1;
			
			filter.reset(new char[s2]);
			HEncoder::FetchEncoder(enc_Native)->
				EncodeFromUTF8(inFilter.c_str(), s1, filter.get(), s2);
			
			ofn.lpstrFilter = filter.get();
			ofn.nFilterIndex = static_cast<unsigned long>(ioFilterIndex);
		}
		
		std::string dir(nil);
		if (inInitialDir)
		{
			dir = inInitialDir->GetCharPath();
			ofn.lpstrInitialDir = dir.c_str();
		}
		
		HWTStr title(inTitle);
		ofn.lpstrTitle = title;

		ofn.hwndOwner = ::GetActiveWindow ();
		ofn.hInstance = gApp->impl ()->GetInstanceHandle ();
		
		if (::GetSaveFileNameA(&ofn))
		{
			outFile = HFileSpec(path.get());
			ioFilterIndex = ofn.nFilterIndex;
			result = true;
		}
	}

	return result;
}

BOOL 
HSHBrowseForFolder(HFileSpec* inInitialDir, const char* inTitle,
	DWORD inFlags, HFileSpec& outDir)
{
	static LPMALLOC sMalloc = nil;
	if (sMalloc == nil)
		::SHGetMalloc(&sMalloc);

	BOOL result = false;
	if (HasUnicode())
	{
		BROWSEINFOW bi = { 0 };
		
		std::wstring dir;
		HWTStr title(inTitle);
		
		if (inInitialDir)
		{
			dir = inInitialDir->GetWCharPath();
			bi.pszDisplayName = const_cast<wchar_t*>(dir.c_str());
		}

		bi.lpszTitle = title;
		bi.ulFlags = inFlags;
		
		LPITEMIDLIST pidlBrowse = ::SHBrowseForFolderW(&bi);
		if (pidlBrowse)
		{
			HAutoBuf<wchar_t> p(new wchar_t[MAX_PATH]);
			::SHGetPathFromIDListW(pidlBrowse, p.get());
			outDir = HFileSpec(p.get());
			if (sMalloc) sMalloc->Free(pidlBrowse);
			result = true;
		}
	}
	else
	{
		BROWSEINFOA bi = { 0 };
		
		std::string dir;
		HWTStr title(inTitle);
		
		if (inInitialDir)
		{
			dir = inInitialDir->GetCharPath();
			bi.pszDisplayName = const_cast<char*>(dir.c_str());
		}

		bi.lpszTitle = title;
		bi.ulFlags = inFlags;
		
		LPITEMIDLIST pidlBrowse = ::SHBrowseForFolderA(&bi);
		if (pidlBrowse)
		{
			HAutoBuf<char> p(new char[PATH_MAX]);
			::SHGetPathFromIDListA(pidlBrowse, p.get());
			outDir = HFileSpec(p.get());
			if (sMalloc) sMalloc->Free(pidlBrowse);
			result = true;
		}
	}

	return result;
}

long HRegOpenKeyEx(HKEY hKey, const char* lpSubKey, DWORD ulOptions,
			REGSAM samDesired, PHKEY phkResult)
{
	long result;
	if (HasUnicode())
		result = ::RegOpenKeyExW(hKey, HWTStr(lpSubKey),
			ulOptions, samDesired, phkResult);
	else
		result = ::RegOpenKeyExA(hKey, HWTStr(lpSubKey),
			ulOptions, samDesired, phkResult);
	return result;
}

long HRegCreateKeyEx(HKEY hKey, const char* lpSubKey, DWORD Reserved,
			const char* lpClass, DWORD dwOptions, REGSAM samDesired, 
			LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, 
			LPDWORD lpdwDisposition)
{
	long result;
	if (HasUnicode())
		result = ::RegCreateKeyExW(hKey, HWTStr(lpSubKey),
			Reserved, HWTStr(lpClass), dwOptions, samDesired,
			lpSecurityAttributes, phkResult, lpdwDisposition);
	else
		result = ::RegCreateKeyExA(hKey, HWTStr(lpSubKey),
			Reserved, HWTStr(lpClass), dwOptions, samDesired,
			lpSecurityAttributes, phkResult, lpdwDisposition);
	return result;
}

long HRegQueryValueEx(HKEY hKey, const char* lpValueName,
			LPDWORD lpReserved, LPDWORD lpType, char* lpData,
			LPDWORD lpcbData)
{
	long result;
	if (HasUnicode())
		result = ::RegQueryValueExW(hKey, HWTStr(lpValueName),
			lpReserved, lpType, (LPBYTE)(wchar_t*)HWFStr(lpData, lpcbData), lpcbData);
	else
		result = ::RegQueryValueExA(hKey, HWTStr(lpValueName),
			lpReserved, lpType, (LPBYTE)(char*)HWFStr(lpData, lpcbData), lpcbData);
	if (lpcbData)
		*lpcbData = std::strlen(lpData);
	return result;	
}

long HRegSetValueEx(HKEY hKey, const char* lpValueName,
			DWORD Reserved, DWORD dwType, const char* lpData)
{
	long result;
	HWTStr data(lpData);
	
	if (HasUnicode())
		result = ::RegSetValueExW(hKey, HWTStr(lpValueName),
			Reserved, dwType, (LPBYTE)(wchar_t*)data,
			2UL * (::lstrlenW(data) + 1));
	else
		result = ::RegSetValueExA(hKey, HWTStr(lpValueName),
			Reserved, dwType, (LPBYTE)(char*)data,
			::lstrlenA(data) + 1UL);
	return result;	
}

long HRegEnumValue(HKEY hKey, DWORD dwIndex, char* lpValueName,
			LPDWORD lpcbValueName, LPDWORD lpReserved,
			LPDWORD lpType, char* lpData, LPDWORD lpcbData)
{
	long result;
	if (HasUnicode())
		result = ::RegEnumValueW(hKey, dwIndex, HWFStr(lpValueName, lpcbValueName),
			lpcbValueName, lpReserved, lpType,
			(LPBYTE)(wchar_t*)HWFStr(lpData, lpcbData), lpcbData);
	else
		result = ::RegEnumValueA(hKey, dwIndex, HWFStr(lpValueName, lpcbValueName),
			lpcbValueName, lpReserved, lpType,
			(LPBYTE)(char*)HWFStr(lpData, lpcbData), lpcbData);
	if (lpcbData)
		*lpcbData = std::strlen(lpData);
	*lpcbValueName = std::strlen(lpValueName);
	return result;	
}

long HRegDeleteValue(HKEY hKey, const char* lpValueName)
{
	long result;
	if (HasUnicode())
		result = ::RegDeleteValueW(hKey, HWTStr(lpValueName));
	else
		result = ::RegDeleteValueA(hKey, HWTStr(lpValueName));
	return result;	
}

long HRegDeleteKey(HKEY hKey, const char* lpKeyName)
{
	long result;
	if (HasUnicode())
		result = ::RegDeleteKeyW(hKey, HWTStr(lpKeyName));
	else
		result = ::RegDeleteKeyA(hKey, HWTStr(lpKeyName));
	return result;	
}

void HCertGetNameString(PCCERT_CONTEXT pCertContext, DWORD dwType,
	DWORD dwFlags, void* pvTypePara, std::string& outName)
{
	if (HasUnicode())
	{
		int c = ::CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara, nil, 0);
		
		HAutoBuf<wchar_t> b(new wchar_t[c + 1]);
		c = ::CertGetNameStringW(pCertContext, dwType, dwFlags, pvTypePara, b.get(), c);
		
		outName = HEncoder::EncodeToUTF8(b.get());
	}
	else
	{
		assert(false);
//		result = ::FormatMessageA(inFlags, (char*)HWTStr(inSource),
//			inMessageID, inLanguageID, (char*)dst,
//			static_cast<unsigned int>(inBufferLength), inArguments);
	}
}

#endif

DWORD HFormatMessage(DWORD inFlags, const char* inSource,
			DWORD inMessageID, DWORD inLanguageID, char* outBuffer,
			int inBufferLength, char** inArguments)
{
	DWORD result;
//	HWFStr dst(outBuffer, static_cast<unsigned int>(inBufferLength));
	HAutoBuf<wchar_t> b(new wchar_t[inBufferLength]);

	UTF16String s(Convert(UTF8String(inSource)));

	//if (HasUnicode())
	result = ::FormatMessageW(inFlags, s.c_str(),
		inMessageID, inLanguageID, b.get(),
		static_cast<unsigned int>(inBufferLength), inArguments);

	UTF8String b2 = Convert(UTF16String(b.get()));
	
	if (b2.length() >= inBufferLength)
		b2.erase(b2.begin() + inBufferLength - 1, b2.end());
			
	b2.copy(outBuffer, inBufferLength - 1);
	outBuffer[b2.length()] = 0;

	//else
	//	result = ::FormatMessageA(inFlags, (char*)HWTStr(inSource),
	//		inMessageID, inLanguageID, (char*)dst,
	//		static_cast<unsigned int>(inBufferLength), inArguments);
	return result;
}

