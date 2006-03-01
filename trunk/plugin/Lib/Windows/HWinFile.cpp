/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created by Bas Vodde Friday January 25 2002 09:03:00
*/

#include "HLib.h"

#define NOMINMAX
#include <windows.h>
#include <shlobj.h>
#include <sstream>

#include "HStdCFcntl.h"
#include "HStdCString.h"
#include "HStdCStdio.h"
#include "HStlSet.h"
#include "HStlVector.h"
#include "HStlStack.h"
#include "HFile.h"
#include "HError.h"
#include "HWinUtils.h"
#include "HUtils.h"
//#include "HEncoder.h"
#include "HDefines.h"
#include "HGlobals.h"
#include "HStrings.h"
#include "HWinApiWrappers.h"
//#include "HAlerts.h"

namespace HFile
{

bool
IsTextFile(const HUrl& inUrl)
{
	HExtensionSet& s = HExtensionSet::Instance();
	
	HExtensionSet::iterator i = s.begin();
	
	// first is *.txt
	// second is *
	// third and next are the language patterns
	
	if (i == s.end())
		return false;
	
	if ((*i).Matches(inUrl))
		return true;
	
	++i;
	if (i == s.end())
		return false;
	
	for (++i; i != s.end(); ++i)
		if ((*i).Matches(inUrl))
			return true;

	return false;
}

#pragma mark -

//struct HIterHelperA
//{
//	typedef	WIN32_FIND_DATAA	find_data;
//	
//	static HANDLE	FindFirstFile(const HFileSpec& inFileName, find_data& inData);
//	static BOOL		FindNextFile(HANDLE inHandle, find_data& inData)
//						{ return ::FindNextFileA(inHandle, &inData); }
//	static char*	GetName(const find_data& inData, char* outName, int inNameLen)
//					{ 
//						unsigned long s1, s2;
//						s1 = static_cast<unsigned long>(::lstrlenA(inData.cFileName) + 1);
//						s2 = static_cast<unsigned long>(inNameLen);
//						
//						HEncoder::FetchEncoder(enc_Native)->
//							EncodeToUTF8(inData.cFileName, s1, outName, s2);
//					
//						return outName;
//					}
//};
//
////HANDLE HIterHelperA::FindFirstFile(const HFileSpec& inFileName, find_data& inData)
////{
////	std::string path(inFileName.GetCharPath());
////	return ::FindFirstFileA(path.c_str(), &inData);
////}
//
//struct HIterHelperW
//{
//	typedef	WIN32_FIND_DATAW	find_data;
//	
//	static HANDLE	FindFirstFile(const HFileSpec& inFileName, find_data& inData);
//	static BOOL		FindNextFile(HANDLE inHandle, find_data& inData)
//						{ return ::FindNextFileW(inHandle, &inData); }
//	static char*	GetName(const find_data& inData, char* outName, int inNameLen)
//					{
//						unsigned long s1, s2;
//						s1 = static_cast<unsigned long>(2 * (::lstrlenW(inData.cFileName) + 1));
//						s2 = static_cast<unsigned long>(inNameLen);
//						
//						HEncoder::FetchEncoder(enc_UTF16LE)->
//							EncodeToUTF8((const char*)inData.cFileName, s1, outName, s2);
//					
//						return outName;
//					}
//};
//
//HANDLE HIterHelperW::FindFirstFile(const HFileSpec& inFileName, find_data& inData)
//{
//	std::wstring path(inFileName.GetWCharPath());
//	return ::FindFirstFileW(path.c_str(), &inData);
//}
//
//struct IteratorImp
//{
//	HUrl				fDirUrl;
//	bool				fValid;
//	
//						IteratorImp(const HUrl& inUrl);
//    virtual				~IteratorImp() {}
//    
//	virtual bool		Next(HUrl& outUrl, const char* inMimeType) = 0;
//	static IteratorImp*	Create(const HUrl& inURL, bool inRecursive);
//};
//
//IteratorImp::IteratorImp(const HUrl& inUrl)
//	: fDirUrl(inUrl)
//{
//	fValid = fDirUrl.IsDirectory();
//}
//
//template<class helper>
//struct BaseIteratorImp : public IteratorImp
//{
//	typedef typename helper::find_data iter_data;
//	
//	iter_data			fIterData;
//	
//						BaseIteratorImp(const HUrl& inUrl)
//							: IteratorImp(inUrl) {}
//    
//    char*				GetName(char* outName, int inNameLen) const;
//	bool				GetURL(HUrl& outUrl, const char* inMimetype) const;
//};
//
//template<class helper>
//char* BaseIteratorImp<helper>::GetName(char* outName, int inNameLen) const
//{
//	return helper::GetName(fIterData, outName, inNameLen);
//}
//
//template<class helper>
//bool BaseIteratorImp<helper>::GetURL(HUrl& outUrl, const char* inMimeType) const
//{
//	char name[NAME_MAX];
//
//	GetName(name, NAME_MAX);
//
//	bool result = true;
//	
//	if (std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0)
//		result = false;
//	else
//	{
//		outUrl = fDirUrl.GetChild(name);
//		
//		if (inMimeType != nil)
//		{
//			if (strncmp(inMimeType, "text", 4) == 0)
//				result = IsTextFile(outUrl);
//			else if (strncmp(inMimeType, "application", 11) == 0)
//			{
//				static HFileExt dlls("apps", "*.DLL;*.EXE");
//				result = dlls.Matches(outUrl);
//			}
//		}
//	}
//	
//	return result;
//}
//
//template<class helper>
//struct FlatIteratorImp : public BaseIteratorImp<helper>
//{
//	HANDLE				fIterHandle;
//	bool				fDone;
//	
//						FlatIteratorImp(const HUrl& inDirURL);
//						~FlatIteratorImp();
//	virtual bool		Next(HUrl& outUrl, const char* inMimeType);
//};
//
//template<class helper>
//FlatIteratorImp<helper>::FlatIteratorImp(const HUrl& inDirURL)
//	: BaseIteratorImp<helper>(inDirURL)
//	, fIterHandle(nil)
//	, fDone(false)
//{
//}
//
//template<class helper>
//FlatIteratorImp<helper>::~FlatIteratorImp()
//{
//	if (fIterHandle != nil)
//		::FindClose(fIterHandle);
//}
//
//template<class helper>
//bool FlatIteratorImp<helper>::Next(HUrl& outUrl, const char* inMimeType)
//{
//	if (fIterHandle == nil)
//	{
//		HFileSpec sp;
//		fDirUrl.GetChild("*").GetSpecifier(sp);
//		fIterHandle = helper::FindFirstFile(sp, fIterData);
//		if (fIterHandle == nil)
//			return false;
//	}
//	else if (not helper::FindNextFile(fIterHandle, fIterData))
//	{
//		assert (GetLastError () == ERROR_NO_MORE_FILES);
//#if P_DEBUG
//		::SetLastError (0);
//#endif
//		return false;
//	}
//
//	while (not GetURL(outUrl, inMimeType))
//	{
//		if (not helper::FindNextFile(fIterHandle, fIterData))
//		{
//			assert (GetLastError () == ERROR_NO_MORE_FILES);
//#if P_DEBUG
//			::SetLastError (0);
//#endif
//			return false;
//		}
//	}
//	
//	return true;
//}
//
//template<class helper>
//struct DeepIteratorImp : public BaseIteratorImp<helper>
//{
//						DeepIteratorImp(const HUrl& inURL);
//						~DeepIteratorImp();
//
//	virtual bool		Next(HUrl& outUrl, const char* inMimeType);
//
//	std::stack<HANDLE>	fDirHandleStack;
//	std::stack<HUrl>	fDirUrlStack;
//	bool				fInited;
//};
//
//template<class helper>
//DeepIteratorImp<helper>::DeepIteratorImp(const HUrl& inDirURL)
//	: BaseIteratorImp<helper>(inDirURL)
//	, fInited(false)
//{
//}
//
//template<class helper>
//DeepIteratorImp<helper>::~DeepIteratorImp()
//{
//	while (fDirHandleStack.size())
//	{
//		::FindClose(fDirHandleStack.top());
//		fDirHandleStack.pop();
//	}
//}
//
//template<class helper>
//bool DeepIteratorImp<helper>::Next(HUrl& outUrl, const char* inMimeType)
//{
//	while (not fInited || fDirHandleStack.size() > 0)
//	{
//		bool pop = true;
//		
//		for (;;)
//		{
//			if (fInited)
//			{
//				if (not helper::FindNextFile(
//					fDirHandleStack.top(), fIterData))
//				{
//					break;
//				}
//			}
//			else
//			{
//				HFileSpec sp;
//				fDirUrl.GetChild("*").GetSpecifier(sp);
//				
//				HANDLE curDir = helper::FindFirstFile(sp, fIterData);
//				if (curDir == nil)
//					ThrowIfOSErr((::GetLastError()));
//				
//				fDirHandleStack.push(curDir);
//				fDirUrlStack.push(fDirUrl);
//				
//				fInited = true;
//			}
//
//			if (fIterData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//			{
//				char name[NAME_MAX];
//				GetName(name, NAME_MAX);
//				
//				if (std::strcmp(name, ".") && std::strcmp(name, ".."))
//				{
//					fDirUrl = fDirUrl.GetChild(name);
//					fInited = false;
//					pop = false;
//					break;
//				}				
//			}
//			else if (GetURL(outUrl, inMimeType))
//				return true;
//		}
//				
//		if (pop)
//		{
//			::FindClose(fDirHandleStack.top());
//			fDirHandleStack.pop();
//			fDirUrlStack.pop();
//			if (fDirUrlStack.size() > 0)
//				fDirUrl = fDirUrlStack.top();
//		}
//	}
//
//	return false;
//}
//
//IteratorImp* IteratorImp::Create(const HUrl& inURL, bool inRecursive)
//{
//	if (HasUnicode())
//	{
//		if (inRecursive)
//			return new DeepIteratorImp<HIterHelperW>(inURL);
//		else
//			return new FlatIteratorImp<HIterHelperW>(inURL);
//	}
//	else
//	{
//		if (inRecursive)
//			return new DeepIteratorImp<HIterHelperA>(inURL);
//		else
//			return new FlatIteratorImp<HIterHelperA>(inURL);
//	}
//}
//
//FileIterator::FileIterator(const HUrl& inDirURL, bool inRecursive)
//	: fImpl(IteratorImp::Create(inDirURL, inRecursive))
//{
//}
//
//FileIterator::~FileIterator()
//{
//	delete fImpl;
//}
//
//bool FileIterator::NextText(HUrl& outUrl)
//{
//	bool result = false;
//	if (fImpl->fValid)
//		result = fImpl->Next(outUrl, "text");
//	return result;
//}
//
//bool FileIterator::Next(HUrl& outUrl, const char* inMimeType)
//{
//	bool result = false;
//	if (fImpl->fValid)
//		result = fImpl->Next(outUrl, inMimeType);
//	return result;
//}
//
//#pragma mark -
//
//struct ChooserImp
//{
//						ChooserImp();
//	bool				Next(HUrl& outURL);
//
//
//	std::vector<HUrl>	fChosen;
//	int					fIndex;
//	std::string			fTitle;
//	bool				fAllowReadOnly;
//	bool				fIsReadOnly;
//};
//
//ChooserImp::ChooserImp()
//	: fIndex(-1)
//	, fTitle(HStrings::GetIndString(4001, 41))
//	, fAllowReadOnly(false)
//	, fIsReadOnly(false)
//{
//}
//
//bool ChooserImp::Next(HUrl& outUrl)
//{
//	if (fIndex < 0 || fIndex >= fChosen.size())
//		return false;
//	outUrl = fChosen[static_cast<unsigned int>(fIndex++)];
//	return true;
//}
//
//Chooser::Chooser()
//	: fImpl(new ChooserImp)
//{
//}
//
//Chooser::~Chooser()
//{
//	delete fImpl;
//}
//
//void
//Chooser::SetTitle(std::string inTitle)
//{
//	fImpl->fTitle = inTitle;
//}
//
//void
//Chooser::SetAllowOpenReadOnly(bool inAllow)
//{
//	fImpl->fAllowReadOnly = inAllow;
//}
//
//bool
//Chooser::OpenReadOnly()
//{
//	return fImpl->fIsReadOnly;
//}
//
//bool		
//Chooser::AskChooseOneFile(HUrl& outUrl, const char* inMimeType)
//{
//	bool result = false;
//
//	DWORD flags = OFN_EXPLORER;
//	if (not fImpl->fAllowReadOnly)
//		flags |= OFN_HIDEREADONLY;
//		
//	std::string filter;
//	
//	if (inMimeType != nil &&
//		std::strncasecmp(inMimeType, "text", 4) == 0)
//	{
//		HExtensionSet::Instance().GetFilter(filter);
//	}
//		
//	HFileSpec* initialDir = nil, dir;
//	if (outUrl.IsValid())
//	{
//		if (not outUrl.IsDirectory())
//			outUrl = outUrl.GetParent();
//		
//		if (outUrl.GetSpecifier(dir) == kNoError)
//			initialDir = &dir;
//	}
//
//	std::vector<HFileSpec> files;
//	if (HGetOpenFileName(flags, filter, initialDir, fImpl->fTitle.c_str(),
//		files, fImpl->fIsReadOnly))
//	{
//		outUrl.SetSpecifier(files[0]);
//		result = true;
//	}
//
//	return result;
//}
//
//// On Windows we cannot have a choose object common dialog... duh...
//// 
//bool		
//Chooser::AskChooseOneObject(HUrl& outUrl)
//{
//	if (ModifierKeyDown(kOptionKey))
//		return AskChooseFolder(outUrl);
//	else
//		return AskChooseOneFile(outUrl, nil);
//}
//
//bool		
//Chooser::AskChooseFiles(const char* inMimeType)
//{
//	bool result = false;
//
//	DWORD flags = OFN_ALLOWMULTISELECT | OFN_EXPLORER;
//	if (not fImpl->fAllowReadOnly)
//		flags |= OFN_HIDEREADONLY;
//		
//	std::string filter;
//	if (inMimeType != nil &&
//		std::strncasecmp(inMimeType, "text", 4) == 0)
//	{
//		HExtensionSet::Instance().GetFilter(filter);
//	}
//		
//	std::vector<HFileSpec> files;
//	if (HGetOpenFileName(flags, filter, nil, fImpl->fTitle.c_str(),
//		files, fImpl->fIsReadOnly))
//	{
//		for (unsigned int i = 0; i < files.size(); ++i)
//			fImpl->fChosen.push_back(HUrl(files[i]));
//		
//		fImpl->fIndex = 0;
//		result = files.size() > 0;
//	}
//
//	return result;
//}
//
//bool		
//Chooser::AskChooseFolder(HUrl& outUrl)
//{
//	bool result = false;
//	
//
//	HFileSpec sp;
//	HFileSpec* initialDir = nil;
//
//	if (outUrl.GetSpecifier(sp) == kNoError)
//		initialDir = &sp;
//		
//	HFileSpec dir;
//	if (::HSHBrowseForFolder(initialDir, fImpl->fTitle.c_str(),
//		BIF_RETURNONLYFSDIRS, dir))
//	{
//		outUrl.SetSpecifier(dir);
//		result = true;
//	}
//	
//	return result;
//}
//
//bool		
//Chooser::Next(HUrl& outUrl)
//{
//	return fImpl->Next(outUrl);
//}
//
//#pragma mark -
//
//struct DesignatorImp
//{
//	bool		fReplacing;
//	std::string	fTitle;
//};
//
//Designator::Designator()
//	: fImpl(new DesignatorImp)
//{
//	fImpl->fReplacing = false;
////	fImpl->fTitle = HStrings::GetIndString();
//}
//
//Designator::~Designator()
//{
//	delete fImpl;
//}
//
//bool		
//Designator::AskDesignateFile(std::string inName, const char* inMimeType,
//	HUrl& outUrl)
//{
//	bool result = false;
//
//	std::string filter;
//	int filterIndex = 0;
//	
//	if (inMimeType != nil &&
//		std::strncasecmp(inMimeType, "text", 4) == 0)
//	{
//		HExtensionSet::Instance().GetFilter(filter);
//		filterIndex = HExtensionSet::Instance().GetIndex(inName) + 1;
//	}
//
//	HFileSpec* initialDir = nil, dir;
//	if (outUrl.IsValid())
//	{
//		if (not outUrl.IsDirectory())
//			outUrl = outUrl.GetParent();
//		
//		if (outUrl.GetSpecifier(dir) == kNoError)
//			initialDir = &dir;
//	}
//
//	HFileSpec file;
//	if (HGetSaveFileName(inName.c_str(), OFN_EXPLORER | OFN_OVERWRITEPROMPT,
//		filter, filterIndex, initialDir, fImpl->fTitle.c_str(), file))
//	{
//		result = true;
//
//		outUrl.SetSpecifier(file);
//		fImpl->fReplacing = Exists(outUrl);
//		
//		std::string name = outUrl.GetFileName();
//		if (name.find('.') == std::string::npos and filterIndex != 2)
//		{
//			HFileExt filter = HExtensionSet::Instance()[filterIndex - 1];
//			if (not filter.Matches(name))
//			{
//				name += filter.GetDefaultExtension();
//				outUrl.SetFileName(name);
//				
//				if (HFile::Exists(outUrl) and
//					HAlerts::Alert(nil, 213, name) != 2)
//				{
//					result = false;
//				}
//			}
//		}
//	}
//
//	return result;
//}
//
//bool		
//Designator::IsReplacing() const
//{
//	return fImpl->fReplacing;
//}

#pragma mark -

void MksTemp(HUrl& ioURL)
{
	std::string fileName = ioURL.GetFileName();
	HUrl dir = ioURL.GetParent();
	char ix = '0';
	
	do
	{
		std::stringstream s;
		s << fileName << '~' << ::GetCurrentProcessId() << '_' << ix;
		ioURL = dir.GetChild(s.str());
		++ix;
	}
	while (Exists(ioURL) and ix <= '9');
}

//SafeSaver::SafeSaver(const HUrl& inSpec, bool inBackup, HUrl inBackupDir)
//	: fSavedFile(inSpec)
//	, fBackup(inBackup)
//	, fCommited(false)
//	, fUseExchange(false)
//{
//	fTempFile = inSpec;
//	MksTemp(fTempFile);
//	
//	if (fBackup)
//	{
//		std::string backupName = inSpec.GetFileName();
//		
//		struct std::tm tm;
//		::DoubleToTM(::system_time(), tm);
//		char s[128];
//		std::snprintf(s, sizeof(s), "(%4.4d%2.2d%2.2d-%2.2d%2.2d%2.2d)",
//			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
//			tm.tm_hour, tm.tm_min, tm.tm_sec);
//		
//		std::string ext;
//		std::string::size_type extP = backupName.rfind('.');
//		if (extP != std::string::npos)
//		{
//			ext = backupName.substr(extP);
//			backupName.erase(extP, std::string::npos);
//		}
//		
//		backupName += s;
//		backupName += ext;
//		
//		fBackupFile = inBackupDir.GetChild(backupName);
//	}
//
//	int fd = Open(fTempFile, O_RDWR | O_CREAT | O_TRUNC);
//	ThrowIfPOSIXErr(fd);
//	Close(fd);
//}
//
//SafeSaver::~SafeSaver()
//{
//	try
//	{
//		if (not fCommited)
//			Unlink(fTempFile);
//	}
//	catch (...) {}
//}
//
//void		
//SafeSaver::Commit()
//{
//	std::wstring saved, temp, backup;
//	HFileSpec spec;
//	
//	ThrowIfOSErr(fSavedFile.GetSpecifier(spec));
//	saved = spec.GetWCharPath();
//
//	ThrowIfOSErr(fTempFile.GetSpecifier(spec));
//	temp = spec.GetWCharPath();
//	
//	if (fBackup)
//	{
//		HUrl b = fTempFile.GetParent().GetChild(fBackupFile.GetFileName());
//		
//		ThrowIfOSErr(b.GetSpecifier(spec));
//		backup = spec.GetWCharPath();
//
//		fCommited = ::ReplaceFileW(saved.c_str(), temp.c_str(),
//			backup.c_str(), REPLACEFILE_WRITE_THROUGH, nil, nil);
//	}
//	else
//	{
//		fCommited = ::ReplaceFileW(saved.c_str(), temp.c_str(), nil,
//			REPLACEFILE_WRITE_THROUGH, nil, nil);
//	}
//	
//	if (not fCommited)
//		ThrowIfOSErr((::GetLastError()));
//	
//	if (fBackup and
//		not (fBackupFile.GetParent() == fSavedFile.GetParent()))
//	{
//		try
//		{
//			temp = backup;
//			
//			ThrowIfOSErr(fBackupFile.GetSpecifier(spec));
//			backup = spec.GetWCharPath();
//	
//			if (not ::MoveFileW(temp.c_str(), backup.c_str()))
//				THROW((pErrMoveBackupFailed));
//		}
//		catch (std::exception& e)
//		{
//			DisplayError(e);
//		}
//	}
//}

int CreateTempFile(const HUrl& inDirectory, const std::string& inBaseName,
	HUrl& outURL)
{
//	string path = inDirectory.GetPath() + '/' + inBaseName + ".XXXXXXXX";
//
//	int fd = ::mkstemp(const_cast<char*>(path.c_str()));
//	ThrowIfPOSIXErr(fd);
//	
//	outURL.SetScheme(kFileScheme);
//	outURL.SetNativePath(path);
//	
//	return fd;

	char ix = '0';
	
	do
	{
		std::stringstream s;
		s << inBaseName << '~' << ::GetCurrentProcessId() << '_' << ix;
		outURL = inDirectory.GetChild(s.str());
		++ix;
	}
	while (Exists(outURL) and ix <= '9');

	return Open(outURL, O_RDWR | O_CREAT | O_TRUNC);
}

SafeSaver::SafeSaver(const HUrl& inSpec)
	: fSavedFile(inSpec)
	, fTempFile(inSpec)
	, fCommited(false)
{
	int fd = CreateTempFile(inSpec.GetParent(), inSpec.GetFileName(), fTempFile);
	Close(fd);
}

SafeSaver::~SafeSaver()
{
	try
	{
		if (not fCommited)
			Unlink(fTempFile);
	}
	catch (...) {}
}

void		
SafeSaver::Commit()
{
	std::wstring saved, temp;
	HFileSpec spec;
	
	ThrowIfOSErr(fSavedFile.GetSpecifier(spec));
	saved = spec.GetWCharPath();

	ThrowIfOSErr(fTempFile.GetSpecifier(spec));
	temp = spec.GetWCharPath();
	
	fCommited = ::ReplaceFileW(saved.c_str(), temp.c_str(), nil,
		REPLACEFILE_WRITE_THROUGH, nil, nil);
	
	if (not fCommited)
		ThrowIfOSErr((::GetLastError()));
}


#pragma mark -

int				
Open(const HUrl& inUrl, int inFlags)
{
	HFileSpec theFile;

	inUrl.GetSpecifier(theFile);

	unsigned long access = 0;
	unsigned long shareMode = 0;
	
	if (inFlags & O_RDWR)
		access = GENERIC_READ | GENERIC_WRITE;
	else if (inFlags & O_WRONLY)
		access = GENERIC_WRITE;
	else
	{
		access = GENERIC_READ;
		shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	}
	
	unsigned long create = 0;
	if (inFlags & O_CREAT)
		create = OPEN_ALWAYS;
	else
		create = OPEN_EXISTING;

	if ((inFlags & O_TRUNC) && Exists(inUrl))
		create |= TRUNCATE_EXISTING;
	
	unsigned long flags = FILE_ATTRIBUTE_NORMAL;
	
	int fd = reinterpret_cast<int>(::HCreateFile(theFile,
		access, shareMode, nil, create, flags, nil));

	if (fd < 0)
	{
		HErrno = static_cast<int>(::GetLastError());
#if P_DEBUG
		HError err(::GetLastError(), true, true);
#endif
	}
	
	return fd;
}

void			
Close(int inFD)
{
	ThrowIfFalse(::CloseHandle((HANDLE)inFD));
}

void Pipe(int& outReadFD, int& outWriteFD)
{
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = true;

	if (not ::CreatePipe(reinterpret_cast<PHANDLE>(&outReadFD),
					reinterpret_cast<PHANDLE>(&outWriteFD),
					&sa, 0))
	{
		THROW((::GetLastError(), true, true));
	}
}

bool			
Exists(const HUrl& inUrl)
{
	bool result = false;
	HFileSpec sp;
	
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		unsigned long attr = HGetFileAttributes(sp);
		result = (attr != -1);
	}
	
	return result;
}

bool			
IsSameFile(const HUrl& inUrl1, const HUrl& inUrl2)
{
	HFileSpec specA, specB;
	if (inUrl1.GetSpecifier(specA) != kNoError)
		return false;
	if (inUrl2.GetSpecifier(specB) != kNoError)
		return false;
	return specA == specB;
}

bool IsDirectory(const HUrl& inUrl)
{
	assert(false);
	HFileSpec spec;
	inUrl.GetSpecifier(spec);
	return spec.IsDirectory();
}

int MkDir(const HUrl& inDir)
{
	int result = 0;

	HFileSpec sp;
	if (inDir.GetSpecifier(sp) != kNoError ||
		not HCreateDirectory(sp, nil))
	{
		result = -1;
		HErrno = static_cast<int>(::GetLastError());
	}

	return result;
}

char* GetCWD(char* inBuffer, int inBufferLength)
{
	std::string cwd(HGetCurrentDirectory());
	
	WinToUnixPath(cwd);
	
//	if (cwd.get())
		std::snprintf(inBuffer, static_cast<unsigned long>(inBufferLength), "%s", cwd.c_str());
//	else
//		*inBuffer = 0;
	
	return inBuffer;
}

void			
Copy(const HUrl& inFromURL, const HUrl& inToURL)
{
	HFileSpec sp1, sp2;
	
	if (inFromURL.GetSpecifier(sp1) == kNoError &&
		inToURL.GetSpecifier(sp2) == kNoError)
	{
		HCopyFile(sp1, sp2, false);
	}
}

int64 Seek(int inFD, int64 inOffset, int inMode)
{
	LARGE_INTEGER offset, newOffset;
	offset.QuadPart = inOffset;

	bool result = false;
	
	switch (inMode)
	{
		case SEEK_SET:
			result = ::SetFilePointerEx(reinterpret_cast<HANDLE>(inFD),
				offset, &newOffset, FILE_BEGIN);
			break;
		case SEEK_CUR:
			result = ::SetFilePointerEx(reinterpret_cast<HANDLE>(inFD),
				offset, &newOffset, FILE_CURRENT);
			break;
		case SEEK_END:
			result = ::SetFilePointerEx(reinterpret_cast<HANDLE>(inFD),
				offset, &newOffset, FILE_END);
			break;
	}
	
	if (result)
		return newOffset.QuadPart;
	else
		return -1;
}

int64 Tell(int inFD)
{
	LARGE_INTEGER offset = { 0 }, newOffset;
	
	if (not ::SetFilePointerEx(reinterpret_cast<HANDLE>(inFD),
			offset, &newOffset, FILE_CURRENT))
	{
		newOffset.QuadPart = -1;
	}
	
	return newOffset.QuadPart;
}

int32 Write(int inFD, const void* inData, uint32 inSize)
{
	DWORD written;
	if (not ::WriteFile(reinterpret_cast<HANDLE>(inFD), inData, inSize, &written, NULL))
		return -1;
	return written;
}

int32 PWrite(int inFD, const void* inData, uint32 inSize, int64 inOffset)
{
	int64 cur_offset = Tell(inFD);
	Seek(inFD, inOffset, SEEK_SET);
	int32 result = Write(inFD, inData, inSize);
	Seek(inFD, cur_offset, SEEK_SET);
	return result;
}

int32 Read(int inFD, void* inData, uint32 inSize)
{
	DWORD written;
	if (not ::ReadFile(reinterpret_cast<HANDLE>(inFD), inData, inSize, &written, NULL))
		return -1;
	return written;
}

int32 PRead(int inFD, void* inData, uint32 inSize, int64 inOffset)
{
	int64 cur_offset = Tell(inFD);
	Seek(inFD, inOffset, SEEK_SET);
	int32 result = Read(inFD, inData, inSize);
	Seek(inFD, cur_offset, SEEK_SET);
	return result;
}

void Truncate(int inFD, int64 inSize)
{
	int64 cur_offset = Tell(inFD);
	Seek(inFD, inSize, SEEK_SET);
	::SetEndOfFile(reinterpret_cast<HANDLE>(inFD));
	Seek(inFD, cur_offset, SEEK_SET);
}

void			
Tmpfile (HUrl& outUrl, const char* inBaseName)
{
	HFileSpec tempDir, file;
	gTempURL.GetSpecifier(tempDir);

	if (HGetTempFileName(tempDir, inBaseName, file) == 0)
		THROW((::GetLastError(), true, true));
	
	/* Store the temp file */
	outUrl.SetSpecifier(file);
}

int				
Unlink(const HUrl& inUrl)
{
	int result = 0;
	
	HFileSpec sp;
	(void)inUrl.GetSpecifier(sp);
	
	if (not HDeleteFile(sp))
	{
		HErrno = static_cast<int>(::GetLastError());
		result = -1;
	}

	return result;
}

int				
Rename(const HUrl& inFromURL, const HUrl& inToURL)
{
	HFileSpec sp1, sp2;
	int result = -1;
	
	if (inFromURL.GetSpecifier(sp1) == kNoError &&
		inToURL.GetSpecifier(sp2) == kNoError)
	{
		if (HMoveFile(sp1, sp2))
			result = 0;
		else
			HErrno = static_cast<int>(::GetLastError());
	}

	return result;
}

int				
Sync(int inFD)
{
	int result = -1;

	if (::FlushFileBuffers((HANDLE)inFD))
		result = 0;
	else
		HErrno = static_cast<int>(::GetLastError());

	return result;
}

int				
GetCreationTime(const HUrl& inUrl, int64& outTime)
{
	int result = -1;
	
	outTime = 0;
	
	HFileSpec sp;
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		HANDLE theFile = HCreateFile(sp,
			0, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nil);
		if (theFile != INVALID_HANDLE_VALUE)
		{
			FILETIME t;
			if (::GetFileTime(theFile, &t, nil, nil))
			{
				outTime = (int64)t.dwHighDateTime << 32 |
						  (int64)t.dwLowDateTime;
				result = 0;
			}
			::CloseHandle(theFile);
		}
	}
	
	return result;
}

int				
GetModificationTime(const HUrl& inUrl, int64& outTime)
{
	int result = -1;
	
	outTime = 0;
	
	HFileSpec sp;
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		HANDLE theFile = HCreateFile(sp,
			0, FILE_SHARE_READ, nil, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nil);
		if (theFile != INVALID_HANDLE_VALUE)
		{
			FILETIME t;
			if (::GetFileTime(theFile, nil, nil, &t))
			{
				outTime = (int64)t.dwHighDateTime << 32 |
						  (int64)t.dwLowDateTime;
				result = 0;
			}
			::CloseHandle(theFile);
		}
	}
	
	return result;
}

int				
SetModificationTime(const HUrl& inUrl, int64 inTime)
{
	int result = -1;
	
	HFileSpec sp;
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		HANDLE theFile = HCreateFile(sp,
			GENERIC_WRITE, 0, nil, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, nil);
		if (theFile != INVALID_HANDLE_VALUE)
		{
			FILETIME t;
			t.dwHighDateTime = static_cast<unsigned long>(inTime >> 32);
			t.dwLowDateTime = static_cast<unsigned long>(inTime);
			
			if (::SetFileTime(theFile, nil, nil, &t))
				result = 0;

			::CloseHandle(theFile);
		}
	}
	
	return result;
}

bool			
IsReadOnly(const HUrl& inUrl)
{
	bool result = false;
	HFileSpec sp;
	
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		unsigned long attr = HGetFileAttributes(sp);
		result = (attr == -1) || (attr & FILE_ATTRIBUTE_READONLY);
	}
	
	return result;
}

void			
SetReadOnly(const HUrl& inUrl, bool inReadOnly)
{
	HFileSpec sp;
	
	if (inUrl.GetSpecifier(sp) == kNoError)
	{
		unsigned long attr = HGetFileAttributes(sp);
		if (attr != -1 &&
			(inReadOnly != (attr & FILE_ATTRIBUTE_READONLY)))
		{
			attr ^= FILE_ATTRIBUTE_READONLY;
			HSetFileAttributes(sp, attr);
		}
	}
}

int				
CopyAttributes(const HUrl& inFromURL, const HUrl& inToURL, bool inStripCKID)
{
	return 0;
}

int				
OpenAttributes(const HUrl& inUrl, int inPermissions)
{
	int result = -1;
	const char kAttributesName[] = ":PepperSettings";

	std::string name = inUrl.GetFileName() + kAttributesName;
	if (name.length() < NAME_MAX)
	{
		HUrl url(inUrl);
		url.SetFileName(name);
		result = Open(url, inPermissions);
	}
	return result;
}

void			
CloseAttributes(int inFD)
{
	Close(inFD);
}

int				
ReadAttribute(int inFD, unsigned long inType, const char* inAttrName, void* outData, int inDataSize)
{
	return -1;
}

int				
WriteAttribute(int inFD, unsigned long inType, const char* inAttrName, const void* inData, int inDataSize)
{
	return -1;
}

int				
GetAttributeSize(int inFD, unsigned long inType, const char* inName)
{
	return -1;
}

int				
ReadAttribute(int inFD, unsigned long inType, int inID, void* outData, int inDataSize)
{
	int result = -1;
	if (inType == 'Pprs' && inID == 129)
	{
		Seek(inFD, 0, SEEK_SET);
		result = Read(inFD, outData, static_cast<unsigned long>(inDataSize));
	}
	return result;
}

int				
WriteAttribute(int inFD, unsigned long inType, int inID, const void* inData, int inDataSize)
{
	int result = -1;
	if (inType == 'Pprs' && inID == 129)
	{
		Seek(inFD, 0, SEEK_SET);
		result = Write(inFD, inData, static_cast<unsigned long>(inDataSize));
	}
	return result;
}

int				
GetAttributeSize(int inFD, unsigned long inType, int inID)
{
	return -1;
}

void			
SetMimeType(const HUrl& inUrl, const char* inMimeType)
{
//	assert(false);
}

void			
GetMimeType(const HUrl& inUrl, char* outMimeType, int inMaxSize)
{
	*outMimeType = 0;
//	assert(false);
}

bool			
IsExecutable(const HUrl& inUrl)
{
	return IsTextFile(inUrl) || HFileExt("exe", "*.exe").Matches(inUrl);
}

void			
SetExecutable(const HUrl& inUrl, bool inExecutable)
{
}

}

// ------------------------------------------------------

//#include <boost/iostreams/device/mapped_file.hpp>
#include "HStream.h"

//struct HMMappedFileStreamImp : public boost::iostreams::mapped_file
//{
//};

HMMappedFileStream::HMMappedFileStream(HFileStream& inFile, int64 inOffset, int64 inLength)
	: fImpl(nil)
{
	//if (inOffset + inLength > inFile.fSize)
	//	THROW(("Error creating memmory mapped file stream"));
	//
	//int64 pageSize = boost::iostreams::mapped_file::alignment();
	//int64 offset = pageSize * (inOffset / pageSize);
	//uint32 before = inOffset - offset;

	//fImpl = new boost::iostreams::mapped_file(
	//	inFile.GetNativePath(), ios_base::in, inLength, offset);
	//
	//fData = const_cast<char*>(fImpl->const_data()) + before;
	//fLogicalSize = inLength;
	//fPhysicalSize = inLength + before;
	//fPointer = 0;
	//fReadOnly = true;
}

HMMappedFileStream::~HMMappedFileStream()
{
	delete fImpl;
}
