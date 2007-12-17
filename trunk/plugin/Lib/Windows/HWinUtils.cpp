/*	$Id: HWinUtils.cpp 30 2006-04-30 17:36:03Z maarten $
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Friday January 25 2002 12:43:52
*/

#include "HLib.h"

#include "HUtils.h"
#include "HWinUtils.h"
#include "HFile.h"

#include <windows.h>
#include <HtmlHelp.h>
#undef GetTopWindow
#define _MFC_OVERRIDES_NEW
//#ifndef P_BUILDING_ADDON
//#include <crtdbg.h>
//#endif
#include <shlobj.h>

#include <cstring>
#include <cctype>
#include "HGlobals.h"
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
