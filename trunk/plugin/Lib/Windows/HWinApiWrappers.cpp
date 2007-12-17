/*	$Id: HWinApiWrappers.cpp 47 2006-05-21 19:02:30Z maarten $
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde on Thursday January 31 2002 14:40:51
*/


#include "HLib.h"

#include <cstring>

#define NOMINMAX
#include <windows.h>
#include <shlobj.h>

#include "HNativeTypes.h"
#include "HWinUtils.h"
#include "HUtils.h"
#include "HDefines.h"

#include "HWinApiWrappers.h"


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

