/*	$Id: HWinApiWrappers.h,v 1.34 2003/12/12 20:34:15 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde on Thursday January 31 2002 14:40:51
*/

#ifndef HWINAPIWRAPPERS_H
#define HWINAPIWRAPPERS_H

#include "HNativeTypes.h"
#include "HStlVector.h"

/*
	This files wraps some Win API calls to use the HToWinString and
	the HFromWinString instead of the W and A calls. This should 
	simplify the other code.
*/

/* Module + Command Line functions */
HMODULE HGetModuleHandle (const char* lpModuleName);
void	HGetCommandLine(char*& outBuffer);
std::string
		HGetCurrentDirectory();
DWORD	HGetTempFileName(const HFileSpec& inTempDir, const char* inBasename,
			HFileSpec& outTempFile);
void	HGetTempPath(HFileSpec& outTempDir);
void	HGetModuleFile(HMODULE inModule, HFileSpec& outSpec);
void	HSHGetFolderPath(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, HFileSpec& outSpec);

///* Message loop functions */
//BOOL	HGetMessage (LPMSG lpMsg, HWND hWnd,UINT wMsgFilterMin, UINT wMsgFilterMax);
//BOOL	HIsDialogMessage (HWND hWnd, LPMSG lpMsg);
//LONG	HDispatchMessage (CONST MSG *lpmsg);
//BOOL	HPostMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//BOOL	HPeekMessage (LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
//LRESULT HSendMessage (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//
///* Window proc functions */
//LRESULT HDefWindowProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//LRESULT HDefDlgProc (HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//LRESULT HDefFrameProc (HWND hWnd, HWND hWndMDIClient, UINT uMsg, WPARAM wParam, LPARAM lParam);
//LRESULT HDefMDIChildProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//LRESULT HCallWindowProc(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
//
//
///* Window class functions */
//ATOM	HRegisterWindowClassEx (UINT style, WNDPROC lpfnWndProc, int cbClsExtra, int cbWndExtra,
//							 HICON hIcon, HCURSOR hCursor, HBRUSH hbrBackground,
//							 char* lpszMenuName, const char* lpszClassName,
//							 HICON hIconSm);
//BOOL	HGetClassInfoEx (const char* lpszClass); /* Just checks if the Window Class exists! */
//DWORD	HGetClassLong (HWND hWnd, int nIndex);
//
//
///* Window functions */
//HWND	HCreateWindowEx (DWORD dwExStyle, const char* lpClassName, const char* lpWindowName, 
//					  DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent,
//					HMENU hMenu, LPVOID lpParam);
//LONG	HGetWindowLong (HWND hWnd, int nIndex);
//LONG	HSetWindowLong (HWND hWnd, int nIndex, LONG dwNewLong);
//
//void*	HGetWindowProperty(HWND hWnd, const char* inName);
//void	HSetWindowProperty(HWND hWnd, const char* inName, void* inProperty);
//void	HRemoveWindowProperty(HWND hWnd, const char* inName);
//
//int		HGetWindowText (HWND hWnd, char* outString, int inStringSize);
//BOOL	HSetWindowText (HWND hWnd, const char* lpString);
//
///* Cursor functions */
//HCURSOR HLoadCursor(const char* lpCursorName);
//HICON	HLoadIcon(const char* lpIconName);

///* Dialog functions */
//HWND	HCreateDialogIndirectParam (LPCDLGTEMPLATE hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
//INT_PTR HDialogBoxParam(HINSTANCE hInstance, const char* lpTemplateName,
//			HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
//void	HSetDlgItemText(HWND inDialog, int inItem, const char* inText);
//void	HGetDlgItemText(HWND inDialog, int inItem, std::string& outText);
//
///* Menu functions */
//struct HMENUITEMINFO
//{
//	UINT		fMask; 
//	UINT		fType; 
//	UINT		fState; 
//	UINT		wID; 
//	HMENU		hSubMenu; 
//	HBITMAP		hbmpChecked; 
//	HBITMAP		hbmpUnchecked; 
//	DWORD		dwItemData; 
//	const char*	dwTypeData; 
//	UINT		cch; 
//};
//
//BOOL	HInsertMenuItem (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii);
//BOOL	HSetMenuItemInfo (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii);
//BOOL	HGetMenuItemInfo (HMENU hMenu, UINT uItem, BOOL fByPosition, HMENUITEMINFO* lpmii);
//
///* Font and text functions */
//DWORD	HGetCharacterPlacement (HDC inDC, const char* inText, int inTextLength,
//			int inMaxExtent, GCP_RESULTSW& outResults, DWORD inFlags);
//BOOL	HExtTextOut (HDC hdc, int nXStart, int nYStart, UINT inOptions, // HTextOut expects an utf8 buffer and byte count
//			CONST RECT *inClipRect, wchar_t* lpString, UINT cbString, CONST INT *lpDx);
//BOOL	HGetTextExtentPoint32 (HDC hdc, const char* lpString, int cbString, HPoint& outPt);
//void	HGetTextNameSizeAndFace(HDC hdc, char* outName, int inNameSize,
//			int& outSize, HFontFace& outFace);
//int		HDrawText(HDC hdc, const char* lpString, int cbString, HRect& ioRect, UINT inFormat);

/* File functions */
HANDLE	HCreateFile(const HFileSpec& inFileName, DWORD dwDesiredAccess,
			DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
			DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
			HANDLE hTemplateFile);
BOOL	HCreateDirectory(const HFileSpec& inDir, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
void	HCopyFile(const HFileSpec& inFile1, const HFileSpec& inFile2, bool inFailIfExists);
DWORD	HGetFileAttributes(const HFileSpec& inFile);
BOOL	HSetFileAttributes(const HFileSpec& inFile, DWORD inAttributes);
BOOL	HDeleteFile(const HFileSpec& inFile);
BOOL	HMoveFile(const HFileSpec& inFileFrom, const HFileSpec& inFileTo);
BOOL	HGetOpenFileName(DWORD inFlags, std::string& inFilter, const HFileSpec* inInitialDir,
			const char* inTitle, std::vector<HFileSpec>& outFiles, bool& outReadOnly);
BOOL	HGetSaveFileName(const char* inName, DWORD inFlags, std::string& inFilter,
			int& ioFilterIndex, const HFileSpec* inInitialDir, const char* inTitle,
			HFileSpec& outFile);
BOOL	HSHBrowseForFolder(HFileSpec* inInitialDir, const char* inTitle, DWORD inFlags,
			HFileSpec& outDir);

///* Registry functions, values are utf8 strings */
//
//long	HRegOpenKeyEx(HKEY hKey, const char* lpSubKey, DWORD ulOptions,
//			REGSAM samDesired, PHKEY phkResult);
//long	HRegCreateKeyEx(HKEY hKey, const char* lpSubKey, DWORD Reserved,
//			const char* lpClass, DWORD dwOptions, REGSAM samDesired, 
//			LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, 
//			LPDWORD lpdwDisposition);
//long	HRegQueryValueEx(HKEY hKey, const char* lpValueName,
//			LPDWORD lpReserved, LPDWORD lpType, char* lpData,
//			LPDWORD lpcbData);
//long	HRegSetValueEx(HKEY hKey, const char* lpValueName,
//			DWORD Reserved, DWORD dwType, const char* lpData);
//long	HRegEnumValue(HKEY hKey, DWORD dwIndex, char* lpValueName,
//			LPDWORD lpcbValueName, LPDWORD lpReserved,
//			LPDWORD lpType, char* lpData, LPDWORD lpcbData);
//long	HRegDeleteValue(HKEY hKey, const char* lpValueName);
//long	HRegDeleteKey(HKEY hKey, const char* lpKeyName);
//
/*  misc */
DWORD	HFormatMessage(DWORD inFlags, const char* inSource,
			DWORD inMessageID, DWORD inLanguageID, char* outBuffer,
			int inBufferLength, char** inArguments);
//void	HCertGetNameString(PCCERT_CONTEXT pCertContext,
//			DWORD dwType, DWORD dwFlags, void* pvTypePara,
//			std::string& outName);

#endif // HWINAPIWRAPPERS_H
