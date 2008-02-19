/*	$Id: HWinNativeTypes.cpp 30 2006-04-30 17:36:03Z maarten $
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Friday January 11 2002 09:35:51
*/

#include "HLib.h"

#define NOMINMAX
#include <windows.h>
#undef GetNextWindow
#undef GetTopWindow
#undef CreateWindow
#undef CreateStatusWindow

#include "HNativeTypes.h"
#include <cstring>
#include <cctype>
#include <string>
#include "HUtils.h"
#include "HEncoding.h"
#include "HWinUtils.h"
#include "HWinApiWrappers.h"
#include "HError.h"

#if P_DEBUG
bool HFileSpec::valid = true;
#endif

HFileSpec::HFileSpec()
{
}

HFileSpec::HFileSpec(std::string inNativePath)
	: fNativePath(inNativePath)
{
}

HFileSpec::HFileSpec(const wchar_t* inNativePath)
{
	SetNativePath(Convert(UTF16String(inNativePath)));
}

HFileSpec::HFileSpec(const HFileSpec& inParent, const char* inRelativePath)
	: fNativePath(inParent.fNativePath)
{
	std::string path;
	GetPath(path);
	if (path[path.length() - 1] != '/')
		path += '/';
	path += inRelativePath;
	SetPath(path);
}

HFileSpec::HFileSpec(const HFileSpec& inParent, const wchar_t* inRelativePath)
	: fNativePath(inParent.fNativePath)
{
	UTF8String child = Convert(UTF16String(inRelativePath));

	std::string path;
	GetPath(path);
	if (path[path.length() - 1] != '/')
		path += '/';
	SetPath(path + child);
}

HFileSpec HFileSpec::GetParent() const
{
	std::string path(fNativePath);
	
	std::string::size_type n;
	if ((n = path.rfind('\\')) != std::string::npos)
		path.erase(n, path.length() - n);
	
	return HFileSpec(path);
}

bool HFileSpec::IsValid() const
{
	return
		fNativePath.length() >= 2 &&
		((std::isalpha(fNativePath[0]) && fNativePath[1] == ':') ||
		 (fNativePath[0] == '\\' && fNativePath[1] == '\\'));
}

bool HFileSpec::IsDirectory() const
{
	bool result = false;

	if (HasUnicode())
	{
		std::wstring path(GetWCharPath());
		unsigned long attr = ::GetFileAttributesW(path.c_str());
		result = attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}
	else
	{
		std::string path(GetCharPath());
		unsigned long attr = ::GetFileAttributesA(path.c_str());
		result = attr != -1 && (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
	}

	return result;
}

void HFileSpec::SetNativePath(std::string inPath)
{
	HWTStr path(inPath.c_str());

	assert(HasUnicode());

	unsigned long size = _MAX_PATH;
	unsigned long r;
	HAutoBuf<wchar_t> fullPath(new wchar_t[size]);
	wchar_t* filePart;
	
	size = ::GetFullPathNameW(path, size, fullPath.get(), &filePart);
	if (size > _MAX_PATH)
	{
		fullPath.reset(new wchar_t[size]);
		r = ::GetFullPathNameW(path, size, fullPath.get(), &filePart);
		if (r == 0 || r > size)
			THROW((::GetLastError(), true, true));
	}

	unsigned long longSize = ::GetLongPathNameW(fullPath.get(), fullPath.get(), size);
	if (longSize == 0)
		longSize = size;
	else if (longSize > size)
	{
		HAutoBuf<wchar_t> longPath(new wchar_t[longSize]);
		r = ::GetLongPathNameW(fullPath.get(), longPath.get(), longSize);
		if (r == 0 || r > longSize)
			THROW((::GetLastError(), true, true));
		fullPath.reset(longPath.release());
	}
	
	fNativePath = Convert(UTF16String(fullPath.get()));
	
	if (fNativePath.compare(0, 8, "\\\\?\\UNC\\") == 0)
		fNativePath.erase(2, 6);
	else if (fNativePath.compare(0, 4, "\\\\?\\") == 0)
		fNativePath.erase(0, 4);
}

HErrorCode HFileSpec::SetPath(std::string inPath)
{
	::UnixToWinPath(inPath);
	SetNativePath(inPath);
	return 0;
}

void HFileSpec::GetPath(std::string& outPath) const
{
	outPath = fNativePath;
	::WinToUnixPath(outPath);
}

void HFileSpec::GetNativePath(std::string& outPath) const
{
	outPath = fNativePath;
}

std::wstring HFileSpec::GetWCharPath() const
{
	std::string path;
	
	if (fNativePath.length() > 2)
	{
		if (std::isalpha(fNativePath[0]) && fNativePath[1] == ':')
		{
			path = "\\\\?\\";
		}
//		else if (fNativePath[0] == '\\' && fNativePath[1] == '\\')
//		{
//			path = "\\\\?\\UNC\\";
//		}
	}
	
	path += fNativePath;
	return Convert(path);
}

std::string HFileSpec::GetCharPath() const
{
#pragma message("plain wrong, of course...")
	return fNativePath;
	//std::string path;
	//path += fNativePath;
	//
	//unsigned long s1, s2;
	//s1 = path.length() + 1;
	//s2 = s1 * 2;
	//
	//HAutoBuf<char>	result(new char[s2]);
	//HEncoder::FetchEncoder(enc_Native)->
	//	EncodeFromUTF8(path.c_str(), s1, result.get(), s2);
	//
	//return std::string(result.get());
}

bool HFileSpec::operator==(const HFileSpec& inOther)
{
	bool result = fNativePath == inOther.fNativePath;
	
		// symbolic links? Hard links?
	if (not result and
		fNativePath.length() != 0 and
		inOther.fNativePath.length() != 0)
	{
		HANDLE file1 = HCreateFile(*this,
			0, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nil);
		HANDLE file2 = HCreateFile(inOther,
			0, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nil);
		
		if (file1 != INVALID_HANDLE_VALUE &&
			file2 != INVALID_HANDLE_VALUE)
		{
			BY_HANDLE_FILE_INFORMATION info1, info2;
			
			if (::GetFileInformationByHandle(file1, &info1) &&
				::GetFileInformationByHandle(file2, &info2))
			{
				result =
					info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber &&
					info1.nFileIndexHigh == info2.nFileIndexHigh &&
					info1.nFileIndexLow == info2.nFileIndexLow;
			}
		}
		
		if (file1 != INVALID_HANDLE_VALUE)
			::CloseHandle(file1);
	
		if (file2 != INVALID_HANDLE_VALUE)
			::CloseHandle(file2);
	}

	return result;
}

bool HFileSpec::operator!=(const HFileSpec& inOther)
{
	return not operator==(inOther);
}

#pragma mark -

HWTStr::HWTStr(const char* inUTF8, unsigned long inLength)
	: fUTF8(inUTF8)
	, fLength(inLength)
{
	if (not fLength && fUTF8)
		fLength = std::strlen(fUTF8) + 1;
}

const wchar_t* HWTStr::get_wchar()
{
	const wchar_t* result = nil;
	
	if (fUTF8 != nil)
	{
		if (fBuffer.length() == 0)
			fBuffer = Convert(UTF8String(fUTF8, fLength));
		result = fBuffer.c_str();
	}

	return result;
}

char* HWTStr::get_char()
{
	PRINT(("Error, should not be called"));
	THROW(("runtime error: calling HWTStr::get_char"));
	return nil;
//	if (fBuffer == nil && fUTF8 != nil)
//	{
//		unsigned long s1 = fLength;
//		unsigned long s2 = s1;
//		fBuffer = new char[s2 + 1];
//	
//		HEncoder::FetchEncoder (enc_Native)->
//			EncodeFromUTF8(fUTF8, s1, fBuffer, s2);
//		fBuffer[s2] = 0;
//	}
//	return fBuffer;
}

HWTStr::~HWTStr()
{
}

#pragma mark -

//HWFStr::HWFStr(char* inBuffer, unsigned long inBufferSize)
//	: fBuffer(inBuffer)
//	, fPrivateBuffer(nil)
//	, fLength(inBufferSize)
//	, fUnicode(HasUnicode())
//{
//	assert(fBuffer || fLength == 0);
//}
//
//HWFStr::HWFStr(char* inBuffer, unsigned long* inBufferSize)
//	: fBuffer(inBuffer)
//	, fPrivateBuffer(nil)
//	, fLength(inBufferSize ? *inBufferSize : 0)
//	, fUnicode(HasUnicode())
//{
//	assert(fBuffer || fLength == 0);
//}
//
//HWFStr::~HWFStr()
//{
//	if (fPrivateBuffer && fBuffer)
//	{
//		try
//		{
//			unsigned long s1, s2;
//			if (fUnicode)
//			{
//				s1 = 2 * (::lstrlenW((wchar_t*)fPrivateBuffer) + 1UL);
//				s2 = fLength;
//				HEncoder::FetchEncoder(enc_UTF16LE)->
//					EncodeToUTF8(fPrivateBuffer, s1, fBuffer, s2);
//			}
//			else
//			{
//				s1 = ::lstrlenA(fPrivateBuffer) + 1UL;
//				s2 = fLength;
//				HEncoder::FetchEncoder(enc_Native)->
//					EncodeToUTF8(fPrivateBuffer, s1, fBuffer, s2);
//			}
//		}
//		catch (...)
//		{
//			*fBuffer = 0;
//		}
//	}
//	else if (fBuffer)
//		*fBuffer = 0;
//	
//	delete[] fPrivateBuffer;
//}
//
//wchar_t* HWFStr::get_wchar()
//{
//	assert(fUnicode == HasUnicode());
//	wchar_t* result = reinterpret_cast<wchar_t*>(fPrivateBuffer);
//	if (fBuffer && fPrivateBuffer == nil)
//	{
//		fPrivateBuffer = new char[(fLength * 2) + 1];
//		result = reinterpret_cast<wchar_t*>(fPrivateBuffer);
//	}
//	return result;
//}
//
//char* HWFStr::get_char()
//{
//	assert(fUnicode == HasUnicode());
//	char* result = fPrivateBuffer;
//	if (fBuffer && fPrivateBuffer == nil)
//	{
//		fPrivateBuffer = new char[fLength + 1];
//		result = fPrivateBuffer;
//	}
//	return result;
//}
//
