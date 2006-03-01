/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Friday January 25 2002 12:43:52
*/

#include "HLib.h"

#include "HUtils.h"
#include "HWinUtils.h"
#ifndef MINI_H_LIB
#include "HWinWindowImp.h"
#endif
#include "HFile.h"

#include <windows.h>
#include <HtmlHelp.h>
#undef GetTopWindow
#define _MFC_OVERRIDES_NEW
//#ifndef P_BUILDING_ADDON
//#include <crtdbg.h>
//#endif
#include <shlobj.h>

#include "HStdCString.h"
#include "HStdCCtype.h"
#include "HGlobals.h"
#ifndef MINI_H_LIB
#include "HEncoder.h"
#include "HApplication.h"
#include "HApplicationImp.h"
#include "HPreferences.h"
#endif
#include "HError.h"
#include "HDefines.h"
#include "HWinApiWrappers.h"

using namespace std;

HError::HError(unsigned long inErr, bool, bool)
	: fErrCode(0)
	, fInfo(static_cast<long>(inErr))
{
	using namespace std;

	HFormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		"", inErr, 0, fMessage, 256, nil);
	if (strlen(fMessage) == 0)
		snprintf(fMessage, 256, "unknown windows error: %ld", inErr);
}

HProfile::HProfile()
{
}

HProfile::~HProfile()
{
}

#ifndef MINI_H_LIB
StSaveDC::StSaveDC()
	: fDC(::GetDC(NULL))
	, fState(::SaveDC(HDC(fDC)))
{
}

StSaveDC::StSaveDC(void* inDC)
	: fDC(inDC)
	, fState(::SaveDC(HDC(fDC)))
{
}

StSaveDC::~StSaveDC()
{
	::RestoreDC(HDC(fDC), fState);
	::ReleaseDC(NULL, HDC(fDC));
}
#endif

double system_time()
{
	static double sDiff = -1.0;

	FILETIME tm;
	ULARGE_INTEGER li;
	
	if (sDiff == -1.0)
	{
		SYSTEMTIME st = { 0 };

		st.wDay = 1;
		st.wMonth = 1;
		st.wYear = 1970;

//		ThrowIfFalse(::SystemTimeToFileTime(&st, &tm));
		(void)::SystemTimeToFileTime(&st, &tm);

		li.LowPart = tm.dwLowDateTime;
		li.HighPart = tm.dwHighDateTime;
		
		// Prevent Ping Pong comment. VC cannot convert UNSIGNED int64 to double. SIGNED is ok. (No more long)
		sDiff = static_cast<__int64> (li.QuadPart);
		sDiff /= 1e7;
	}	
	
	::GetSystemTimeAsFileTime(&tm);
	
	li.LowPart = tm.dwLowDateTime;
	li.HighPart = tm.dwHighDateTime;
	
	double result = static_cast<__int64> (li.QuadPart);
	result /= 1e7;
	return result - sDiff;
}

double get_dbl_time()
{
	return 0.5;
}

double get_caret_time()
{
	return 0.5333;
}

void delay(double inSeconds)
{
	if (inSeconds > 0.0)
		::Sleep(static_cast<unsigned long>(inSeconds * 1000));
}

void beep()
{
	::MessageBeep(MB_ICONEXCLAMATION);
}


#ifndef MINI_H_LIB
bool ModifierKeyDown(int inModifier)
{
	unsigned long modifiers;
	GetModifierState(modifiers, inModifier != kAlphaLock);
	return (modifiers & inModifier) != 0;
}

#undef GetUserName
bool GetUserName(std::string& outName)
{
	bool result;
	char name[NAME_MAX];

	{
		HWFStr nm(name, NAME_MAX);
		unsigned long l = NAME_MAX;
		result = ::GetUserNameW(nm, &l) != 0;
		nm.get_wchar()[l] = 0;
	}

	outName = name;
	return result;
}
#endif

bool InitHasUnicode ()
{
	bool supportUnicode = true;
	OSVERSIONINFOW wInfo;
	wInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFOW);
	if (::GetVersionExW (&wInfo) == 0) {
		OSVERSIONINFOA aInfo;
		aInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
		if (::GetVersionExA (&aInfo) == 0) {
			/* Something very wrong */
			assert (false);
		}
		if (aInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
			supportUnicode = false;
		}
		gOSVersion = static_cast<long>(aInfo.dwMajorVersion);
		gOSVersionMinor = static_cast<long>(aInfo.dwMinorVersion);
			
	}
	else {
		if (wInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
			supportUnicode = false;
		}
		gOSVersion = static_cast<long>(wInfo.dwMajorVersion);
		gOSVersionMinor = static_cast<long>(wInfo.dwMinorVersion);
	}
	return supportUnicode;
}

void UnixToWinPath(std::string& ioPath)
{
	NormalizePath(ioPath);

	if (ioPath.length() >= 3 && ioPath[0] == '/' &&
		((std::isalpha(ioPath[1]) && ioPath[2] == ':') ||
		 (ioPath[1] == '/' && ioPath[2] == '/')))
	{
		ioPath.erase(0, 1);
//		
//		if (ioPath.length() >= 3 && ioPath[2] != '/')
//			ioPath.insert(2, "/");
	}

	std::string::size_type i = 0;
	while ((i = ioPath.find('/', i)) != std::string::npos)
		ioPath[i] = '\\';

// hmmm, aparently there are more characters disallowed on Win32	
/*	while ((i = ioPath.find("%5C")) != std::string::npos || (i = ioPath.find("%5C")) != std::string::npos)
	{
		ioPath.erase(i, 3);
		ioPath.insert(i, "/");
	}
*/
}

void WinToUnixPath(std::string& ioPath)
{
	std::string::size_type i = 0;
	
/*	while ((i = ioPath.find('/', i)) != std::string::npos)
	{
		ioPath.erase(i, 1);
		ioPath.insert(i, "%2F");
	}
*/
		// check for absolute path
	if (ioPath.length() >= 2 &&
		((std::isalpha(ioPath[0]) && ioPath[1] == ':') ||
		 (ioPath[0] == '\\' && ioPath[1] == '\\')))
	{
		ioPath.insert(0, "/");
	}
	
	i = 0;
	while ((i = ioPath.find('\\', i)) != std::string::npos)
		ioPath[i] = '/';
	
	NormalizePath(ioPath);
}

#if P_DEBUG

static const char* sDebugFileName;
static int sDebugLineNumber;

void set_debug_info (const char* inFilename, int inLineNumber)
{
	sDebugFileName = inFilename;
	sDebugLineNumber = inLineNumber;
}

void debug_printf(const char* msg, ...)
{
#if 0 //P_VISUAL_CPP
	_CrtDbgReport (_CRT_WARN, sDebugFileName, sDebugLineNumber, NULL, msg);
#else
	using namespace std;
	
	char message[1024];
	
	va_list vl;
	va_start(vl, msg);
	vsnprintf(message, 1024, msg, vl);
	va_end(vl);
	
	::OutputDebugStringA(message);
#endif
}
#endif

//bool IsCancelEventInEventQueue()
//{
//	bool result = false;
//	MSG msg;
//	if (::HPeekMessage(&msg, nil, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
//	{
//		if (msg.message == WM_CHAR)
//		{
//			result =
//				msg.wParam == 'C' - '@' ||		// ctrl-C
//				msg.wParam == 0x1b;				// escape
//		}
//		else
//			::TranslateMessage(&msg);
//	}
//	return result;
//}

#ifndef MINI_H_LIB
void GetModifierState(unsigned long& outModifiers, bool inAsync)
{
	outModifiers = 0;
	if (inAsync)
	{
		if (::GetAsyncKeyState(VK_SHIFT) & 0x8000)
			outModifiers |= kShiftKey;
		if (::GetAsyncKeyState(VK_CONTROL) & 0x8000)
			outModifiers |= kControlKey;
		if (::GetAsyncKeyState(VK_MENU) & 0x8000)
			outModifiers |= kOptionKey;
		if (::GetAsyncKeyState(VK_LWIN) & 0x8000 ||
			::GetAsyncKeyState(VK_RWIN) & 0x8000)
			outModifiers |= kCmdKey;
		if (::GetKeyState(VK_CAPITAL) & 0x0001)
			outModifiers |= kAlphaLock;
	}
	else
	{
		if (::GetKeyState(VK_SHIFT) & 0x8000)
			outModifiers |= kShiftKey;
		if (::GetKeyState(VK_CONTROL) & 0x8000)
			outModifiers |= kControlKey;
		if (::GetKeyState(VK_MENU) & 0x8000)
			outModifiers |= kOptionKey;
		if (::GetKeyState(VK_LWIN) & 0x8000 ||
			::GetKeyState(VK_LWIN) & 0x8000)
			outModifiers |= kCmdKey;
		if (::GetKeyState(VK_CAPITAL) & 0x0001)
			outModifiers |= kAlphaLock;
	}
}

void ShowInExplorer(const HUrl& inUrl)
{
	HFileSpec spec;
	std::string path;

//	inUrl.GetParent().GetSpecifier(spec);
//	spec.GetNativePath(path);
//
//	HINSTANCE h = ::ShellExecuteA(
//		static_cast<HWinWindowImp*>(HWindow::GetTopWindow()->impl())->GetHandle(),
//		"explore", path.c_str(), nil, nil, SW_SHOWNORMAL);

	path = inUrl.GetNativePath();
	std::string cmd = " /e,/select,\"";
	cmd += path;
	cmd += "\"";
	HINSTANCE h = ::ShellExecuteA(
		static_cast<HWinWindowImp*>(HWindow::GetTopWindow()->impl())->GetHandle(),
		"open", "explorer.exe", cmd.c_str(), nil, SW_SHOWNORMAL);

	if (reinterpret_cast<unsigned int>(h) < 32)
		beep();
}

void ShowInWebBrowser(const HUrl& inUrl)
{
	const int kMaxValueLength = 1024;

	// first find out the default value for the ".html" entry.
	HKEY key;
	int err = ::HRegOpenKeyEx(HKEY_CLASSES_ROOT, ".html", 0, KEY_ALL_ACCESS, &key);
	if (err == ERROR_SUCCESS)
	{
		unsigned long size = kMaxValueLength + 1, type;
		HAutoBuf<char> value(new char[size]);

		err = ::HRegQueryValueEx(key, nil, nil, &type, value.get(), &size);
		(void)::RegCloseKey(key);
		
		if (err == ERROR_SUCCESS)
		{
			assert(type == REG_SZ);
			assert(size < kMaxValueLength);
			
			// and now try to get the command to launch our webbrowser
			
			std::string keyStr = "\\";
			keyStr += value.get();
			keyStr += "\\shell\\open\\command";
			
			err = ::HRegOpenKeyEx(HKEY_CLASSES_ROOT, keyStr.c_str(),
				0, KEY_ALL_ACCESS, &key);
			if (err == ERROR_SUCCESS)
			{
				size = kMaxValueLength + 1;
				value.reset(new char[size]);
		
				err = ::HRegQueryValueEx(key, nil, nil, &type, value.get(), &size);
				(void)::RegCloseKey(key);
				
				if (err == ERROR_SUCCESS)
				{
					std::string cmd = value.get();
					std::string::size_type n;
					
					n = cmd.find("%1");
					if (n == std::string::npos)
					{
						cmd += " \"";
						cmd += inUrl.GetURL();
						cmd += '"';
					}
					else
						cmd.replace(n, 2, inUrl.GetURL());
					
					n = cmd.find("%3A/");
					if (n != std::string::npos)
						cmd.replace(n, 4, ":/");
					
					STARTUPINFOA si = { 0 };
					si.cb = sizeof(si);
//					si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;

					PROCESS_INFORMATION pi;
					::CreateProcessA(nil, const_cast<char*>(cmd.c_str()),
						nil, nil, false, 0, nil, nil, &si, &pi);
				}
			}
		}
	}
}

bool UsePassiveFTPMode()
{
	return false;
}

#if P_CODEWARRIOR && __MWERKS__ < 0230
extern "C" 
void itoa(int i, char* a, int b)
{
assert(false);
	std::sprintf(a, "%d", i);
}
#endif

HKEY GetKey(HKEY inParent, const char* inPath, bool inForWriting);
bool GetRegValue(HKEY inParent, const char* inPath, const char* inValue,
	std::string& outValue);
bool SetRegValue(HKEY inParent, const char* inPath, const char* inKey, const char *inValue);

HKEY GetKey(HKEY inParent, const char* inPath, bool inForWriting)
{
	HKEY key = nil;
	DWORD permission;
	if (inForWriting)
		permission = KEY_WRITE;
	else
		permission = KEY_READ;
	
	int err = ::RegOpenKeyEx(inParent, inPath, 0, permission, &key);

	if (err != ERROR_SUCCESS && inForWriting)
	{
		std::string path(inPath);
		std::string::size_type n = path.rfind('\\');
		if (n != std::string::npos)
		{
			path.erase(n, path.length() - n);
			key = GetKey(inParent, path.c_str(), true);
		}
		else
			key = inParent;

		if (key != nil)
		{
			unsigned long disp;
			HKEY newKey;
			
			err = ::RegCreateKeyEx(key, inPath + n + 1,
				0, "", 0, KEY_ALL_ACCESS, nil, &newKey, &disp);

			if (key != inParent)
				::RegCloseKey(key);
			key = newKey;
		}

		if (err == ERROR_SUCCESS)
			err = ::RegOpenKeyEx(inParent, inPath, 0, permission, &key);
	}

	return key;
}

bool GetRegValue(HKEY inParent, const char* inPath, const char* inValue,
	std::string& outValue)
{
	bool result = false;
	HKEY key = GetKey(inParent, inPath, false);
	if (key != nil)
	{
		unsigned long size = 1024, type;
		HAutoBuf<char> value(new char[size]);

		int err = ::RegQueryValueEx(key, inValue, nil, &type, LPBYTE(value.get()), &size);
		(void)::RegCloseKey(key);
		
		outValue = value.get();
		result = (err == ERROR_SUCCESS);
	}
	return result;
}

bool SetRegValue(HKEY inParent, const char* inPath, const char* inKey, const char *inValue)
{
	bool result = false;
	HKEY key = GetKey(inParent, inPath, true);
	if (key != nil)
	{
		int err = RegSetValueEx(key, inKey, 0, REG_SZ, LPBYTE(inValue), strlen(inValue));
		(void)::RegCloseKey(key);
		result = (err == ERROR_SUCCESS);
	}
	return result;
}

void RegisterFileExtension(const char* inExt, const char* inDesc)
{
	std::string quotedPepperPath;

	if (inExt == nil || *inExt != '.')
		THROW((pErrCannotAssocExt, inExt ? inExt : "(null)"));

	if (GetRegValue(HKEY_LOCAL_MACHINE,
		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Pepper.exe",
		nil, quotedPepperPath))
	{
		quotedPepperPath.insert(0, "\"");
		quotedPepperPath.append("\"");

		std::string name = "Pepper";
		name += inExt;
		
		bool add = false;

		std::string key = "Software\\Hekkelman\\pepper\\Associations\\";
		key += inExt;
		
		std::string curAssoc;
		if (not GetRegValue(HKEY_CLASSES_ROOT, inExt, nil, curAssoc) ||
			curAssoc != name)
		{
			if (curAssoc.length() == 0)
				curAssoc = "none";
	
			SetRegValue(HKEY_CURRENT_USER, key.c_str(), nil, curAssoc.c_str());
			add = true;
		}
		
		std::string dummy;
		if (not add && not GetRegValue(HKEY_CURRENT_USER, key.c_str(), nil, dummy))
		{
			SetRegValue(HKEY_CURRENT_USER, key.c_str(), nil, "none");
			add = true;
		}
		
		if (add)
		{
			SetRegValue(HKEY_CLASSES_ROOT,
				inExt, nil, name.c_str());
			SetRegValue(HKEY_CLASSES_ROOT,
				name.c_str(), nil, inDesc);
			SetRegValue(HKEY_CLASSES_ROOT,
				(name + "\\DefaultIcon").c_str(),
				nil, (quotedPepperPath + ",1").c_str());
			SetRegValue(HKEY_CLASSES_ROOT,
				(name + "\\shell\\open\\command").c_str(),
				nil, (quotedPepperPath + " \"%1\"").c_str());
			SetRegValue(HKEY_CLASSES_ROOT,
				(name + "\\shell\\open\\ddeexec").c_str(),
				nil, "[open(\"%1\")]");
			SetRegValue(HKEY_CLASSES_ROOT,
				(name + "\\shell\\open\\ddeexec\\Application").c_str(),
				nil, "PEPPER");
			SetRegValue(HKEY_CLASSES_ROOT,
				(name + "\\shell\\open\\ddeexec\\topic").c_str(),
				nil, "System");
		}
	}
}

void RestoreFileExtension(const char* inExt)
{
	std::string key = "Pepper";
	key += inExt;
	::RegDeleteKey(HKEY_CLASSES_ROOT, key.c_str());
	
	key = "Software\\Hekkelman\\pepper\\Associations\\";
	key += inExt;
	
	std::string prevAssoc;
	if (GetRegValue(HKEY_CURRENT_USER, key.c_str(), nil, prevAssoc))
	{
		if (prevAssoc == "none")
			::RegDeleteKey(HKEY_CLASSES_ROOT, inExt);
		else
			SetRegValue(HKEY_CLASSES_ROOT, inExt, nil, prevAssoc.c_str());

		HKEY hKey = GetKey(HKEY_CURRENT_USER, "Software\\Hekkelman\\pepper\\Associations", false);
		if (hKey != nil)
		{
			::RegDeleteKey(hKey, inExt);
			::RegCloseKey(hKey);
		}
	}
}

bool NextAssoc(unsigned long& ioCookie, std::string& outExt, std::string& outDesc)
{
	static HKEY sKey = nil;
	bool result = false;
	
	if (ioCookie == 0)
	{
		std::string key = "Software\\Hekkelman\\pepper\\Associations";
		sKey = GetKey(HKEY_CURRENT_USER, key.c_str(), false);
	}
	
	if (sKey != nil)
	{
		char name[64];
		DWORD size = 64;
		FILETIME t;
		
		int err = ::RegEnumKeyEx(sKey, ioCookie++, name, &size, nil, nil, nil, &t);
		if (err == ERROR_SUCCESS)
		{
			outExt = name;

			std::string descName = "Pepper";
			descName += name;

			GetRegValue(HKEY_CLASSES_ROOT, descName.c_str(), nil, outDesc);
			result = true;
		}
		else
		{
			::RegCloseKey(sKey);
			sKey = nil;
		}
	}

	return result;
}
#endif

#if 0
char* GetFormattedDate(char* outDate, int inBufferSize)
{
	::GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE,
		nil, nil, outDate, inBufferSize);
	return outDate;
}

char* GetFormattedTime(char* outTime, int inBufferSize)
{
	::GetTimeFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE,
		nil, nil, outTime, inBufferSize);
	return outTime;
}
#endif

bool GetFormattedBool()
{
	std::string appName = kAppName;
	appName += ".exe";
	
	int64 appDate;
	HFile::GetModificationTime(gAppURL.GetChild(appName), appDate);

	SYSTEMTIME st;
	::GetSystemTime(&st);
	
	FILETIME ft;
	::SystemTimeToFileTime(&st, &ft);
	
	int64 now;
	now = ft.dwHighDateTime;
	now <<= 32;
	now |= ft.dwLowDateTime;
	
	double diff = (now - appDate) / 10000.0;
			
	return diff < 0 || diff > (7 * 24 * 3600);
}

std::string GetFormattedFileDateAndTime(int64 inDateTime)
{
	static double sDiff = -1.0;

	if (sDiff == -1.0)
	{
		SYSTEMTIME st = { 0 };

		st.wDay = 1;
		st.wMonth = 1;
		st.wYear = 1970;

		FILETIME tm;
//		ThrowIfFalse(::SystemTimeToFileTime(&st, &tm));
		(void)::SystemTimeToFileTime(&st, &tm);
		
		ULARGE_INTEGER li;
	
		li.LowPart = tm.dwLowDateTime;
		li.HighPart = tm.dwHighDateTime;
		
		// Prevent Ping Pong comment. VC cannot convert UNSIGNED int64 to double. SIGNED is ok. (No more long)
		sDiff = static_cast<__int64> (li.QuadPart);
		sDiff /= 1e7;

#if DEBUG
		double t = inDateTime / 1e7 - sDiff;
		int64 t2 = static_cast<int64>(1e7 * (sDiff + t));
		int64 d = t2 - inDateTime;
#endif		
	}
	
	union
	{
		FILETIME	ft;
		int64		t;
	} fileTime, localFileTime;

	fileTime.t = inDateTime;
	::FileTimeToLocalFileTime(&fileTime.ft, &localFileTime.ft);
	
	return GetFormattedFileDateAndTime(localFileTime.t / 1e7 - sDiff);
}

//void DisplayHelpFile(HUrl inHelpFile)
//{
////	SHELLEXECUTEINFOW ShExecInfo = { 0 };
//	
//	HFileSpec spec;
//	inHelpFile.GetSpecifier(spec);
//
//	std::wstring path = spec.GetWCharPath();
//	
//	if (path.substr(0, 4) == L"\\\\?\\")
//		path.erase(0, 4);
//	
//	path += L"::/Content/main.html";
//
////	std::string path = inHelpFile.GetNativePath();
////	path += "::/Content/main.html";
//
////	std::wstring path = L"C:\\projects\\Pepper\\Documentation\\PepperManual.chm::";
//	
////	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
////	ShExecInfo.lpFile = spec.GetWCharPath();
////	ShExecInfo.nShow = SW_MAXIMIZE;
////	
////	::ShellExecuteExW(&ShExecInfo);
//	HWND w = ::HtmlHelpW(::GetDesktopWindow(), path.c_str(), HH_DISPLAY_TOC, 0);
//}

#if NDEBUG

extern "C" {
_CRTIMP void * __cdecl _malloc_dbg(
        size_t size,
        int,
        const char *,
        int
        )
{
	return ::malloc(size);
}

_CRTIMP void __cdecl _free_dbg(
        void * p,
        int
        )
{
	::free(p);
}

}

#endif


#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"			// must be XP version of file
#else
// VC7: ships with updated headers
#include "dbghelp.h"
#endif

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

class MiniDumper
{
  public:
	MiniDumper();

  private:
	static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo );
};

// create one global object of minidumper:
MiniDumper sMiniDumper;

MiniDumper::MiniDumper()
{
	::SetUnhandledExceptionFilter(TopLevelFilter);
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

	// hope we can find it...
	HMODULE hDll = ::LoadLibrary( "DBGHELP.DLL" );

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			HUrl dump;
			
			int fd = HFile::CreateTempFile(gAppURL.GetParent(),
				(string(kAppName) + ".dmp").c_str(), dump);
			
			if (fd >= 0)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				// write the dump
				(void)pDump(::GetCurrentProcess(), ::GetCurrentProcessId(), (HANDLE)fd, MiniDumpNormal, &ExInfo, NULL, NULL);

				HFile::Close(fd);
			}
		}
	}

	return retval;
}
