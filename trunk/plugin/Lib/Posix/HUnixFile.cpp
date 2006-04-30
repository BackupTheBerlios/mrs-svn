/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday September 12 2000 09:03:00
*/

/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "HLib.h"

#include <stack>
#include <cstdio>
#if P_DEBUG
#include <iostream>
#endif

#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "HFile.h"
#include "HUrl.h"
#include "HUtils.h"
#include "HError.h"
#include "HGlobals.h"
#include "HNativeTypes.h"
#include "HStream.h"

using namespace std;

//extern int _h_errno;

HFileSpec::HFileSpec()
{
}

HFileSpec::HFileSpec(const HFileSpec& inParent, const char* inRelativePath)
{
	assert(false);
}

HFileSpec HFileSpec::GetParent() const
{
	assert(false);
	return *this;
}

void HFileSpec::SetNativePath(string inPath)
{
	assert(false);
}

HErrorCode HFileSpec::SetPath(const string& inPath)
{
	assert(false);
	return -1;
}

void HFileSpec::GetPath(string& outPath) const
{
	assert(false);
}

void HFileSpec::GetNativePath(string& outPath) const
{
	assert(false);
}

namespace HFile
{

struct IteratorImp
{
	virtual			~IteratorImp() {}
	virtual bool	Next(HUrl& outUrl) = 0;
	virtual bool	Next(HUrl& outUrl, const char* ext) = 0;
	virtual bool	NextDir(HUrl& outUrl);
	
	static IteratorImp*	Create(const HUrl& inURL, bool inRecursive);
};

bool IteratorImp::NextDir(HUrl& outUrl)
{
	assert(false);
	return false;
}

struct FlatIteratorImp : public IteratorImp
{
	DIR*			fDir;
	string			fDirPath;
				
					FlatIteratorImp(const string& inDirPath);
					~FlatIteratorImp();
	virtual bool	Next(HUrl& outUrl);
	virtual bool	Next(HUrl& outUrl, const char* ext);
	virtual bool	NextDir(HUrl& outUrl);
};

FlatIteratorImp::FlatIteratorImp(const string& inDirPath)
	: fDir(nil)
	, fDirPath(inDirPath + '/')
{
	fDir = opendir(fDirPath.c_str());
	if (fDir == nil)
		THROW((HError("Could not opendir %s: %s", fDirPath.c_str(), strerror(errno))));
}

FlatIteratorImp::~FlatIteratorImp()
{
	if (fDir != nil)
		closedir(fDir);
}

bool FlatIteratorImp::Next(HUrl& outUrl)
{
	while (true)
	{
		outUrl.SetScheme(kFileScheme);
		struct dirent* e = readdir(fDir);
		
		if (e == nil)
			return false;
		
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;
		
		string path = fDirPath;
		path += e->d_name;

		struct stat st;
		if (stat(path.c_str(), &st) != kNoError)
			continue;
		
		outUrl.SetPath(path);
		break;
	}
	return true;
}

bool FlatIteratorImp::Next(HUrl& outUrl, const char* ext)
{
	int n = ext ? strlen(ext) : 0;

	while (true)
	{
		outUrl.SetScheme(kFileScheme);
		struct dirent* e = readdir(fDir);
		
		if (e == nil)
			return false;
		
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;

		if (ext && strcmp(e->d_name + strlen(e->d_name) - n, ext))
			continue;
		
		string path = fDirPath;
		path += e->d_name;

		struct stat st;
		if (stat(path.c_str(), &st) != kNoError)
			continue;

		outUrl.SetPath(path);
		break;
	}
	return true;
}

bool FlatIteratorImp::NextDir(HUrl& outUrl)
{
	while (true)
	{
		outUrl.SetScheme(kFileScheme);
		struct dirent* e = readdir(fDir);
		
		if (e == nil)
			return false;
		
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;
		
		string path = fDirPath;
		path += e->d_name;

		struct stat st;
		if (stat(path.c_str(), &st) != kNoError)
			continue;
		
		if (not S_ISDIR(st.st_mode))
			continue;

		outUrl.SetPath(path);
		break;
	}
	return true;
}

struct DeepIteratorImp : public IteratorImp
{
	string			fStartDir;
	stack<string>	fNameStack;
	stack<DIR*>		fDIRStack;
	
	string			fCurPath;
	DIR*			fCurDIR;

					DeepIteratorImp(const string& inDirPath);
					~DeepIteratorImp();

	virtual bool	Next(HUrl& outUrl);
	virtual bool	Next(HUrl& outUrl, const char* ext);
};

DeepIteratorImp::DeepIteratorImp(const string& inDirPath)
	: fCurPath(inDirPath + '/')
	, fCurDIR(nil)
{
	fCurDIR = ::opendir(inDirPath.c_str());
	ThrowIfNil(fCurDIR);
}

DeepIteratorImp::~DeepIteratorImp()
{
	while (fDIRStack.size())
	{
		::closedir(fDIRStack.top());
		fDIRStack.pop();
	}
	
	if (fCurDIR)
		::closedir(fCurDIR);
}

bool DeepIteratorImp::Next(HUrl& outUrl)
{
	while (true)
	{
		outUrl.SetScheme(kFileScheme);
		struct dirent* e = ::readdir(fCurDIR);
		
		if (e == nil)	// end of dir, pop stack or return if empty
		{
			if (fDIRStack.empty())
				return false;

			::closedir(fCurDIR);

			fCurDIR = fDIRStack.top();
			fDIRStack.pop();
			
			fCurPath = fNameStack.top();
			fNameStack.pop();
			
			continue;
		}
		
		if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0)
			continue;
		
		string path = fCurPath;
		path += e->d_name;

		struct stat st;
		if (stat(path.c_str(), &st) != kNoError)
			continue;
		
		if (S_ISDIR(st.st_mode) && not S_ISLNK(st.st_mode))
		{
			if (path[path.length() - 1] != '/')
				path += '/';
			
			fNameStack.push(fCurPath);
			fDIRStack.push(fCurDIR);
			
			fCurPath = path;
			fCurDIR = ::opendir(path.c_str());
			
			if (fCurDIR == nil)
			{
				fCurDIR = fDIRStack.top();
				fDIRStack.pop();
				
				fCurPath = fNameStack.top();
				fNameStack.pop();
				
				continue;
			}
		}

		outUrl.SetPath(path);
		break;
	}

	return true;
}

bool DeepIteratorImp::Next(HUrl& outUrl, const char* ext)
{
	string::size_type n = ext ? strlen(ext) : 0;

	while (true)
	{
		if (not Next(outUrl))
			return false;
		
		string name = outUrl.GetFileName();
		
		if (ext && name.compare(n, strlen(ext), ext))
			continue;
		
		break;
	}
	
	return true;
}

IteratorImp* IteratorImp::Create(const HUrl& inURL, bool inRecursive)
{
	string path = inURL.GetPath();
	if (inRecursive)
		return new DeepIteratorImp(path);
	else
		return new FlatIteratorImp(path);
}

FileIterator::FileIterator(const HUrl& inDirURL, bool inRecursive)
	: fImpl(IteratorImp::Create(inDirURL, inRecursive))
{
}

//FileIterator::FileIterator(const char* inDir, bool inRecursive)
//	: fImpl(new FlatIteratorImp(HUrl(inDir)))
//{
//} /* FileIterator::FileIterator */

FileIterator::~FileIterator()
{
	delete fImpl;
}

bool FileIterator::NextText(HUrl& outUrl)
{
	while (fImpl->Next(outUrl, nil))
	{
		if (IsTextFile(outUrl))
			return true;
	}
	return false;
}

bool FileIterator::Next(HUrl& outUrl, const char* inMimeType)
{
	if (inMimeType && strcmp(inMimeType, "directory") == 0)
		return fImpl->NextDir(outUrl);
	else
	{
		const char* ext = nil;
		bool wantText = false;
		
		if (inMimeType == nil || *inMimeType == 0 || strcmp(inMimeType, "*") == 0)
			ext = nil;
		else if (strcmp(inMimeType, "text/x-vnd.hekkelman-peppercachefile") == 0)
			ext = ".swp";
		else if (strcmp(inMimeType, "text") == 0 || strncmp(inMimeType, "text/", 5) == 0)
		{
			wantText = true;
			ext = nil;
		}
		else if (strcmp(inMimeType, "application/x-vnd.pepper-languageplugin") == 0)
			ext = ".so";
		else if (strcmp(inMimeType, "application/x-vnd.hekkelman-pepperaddon") == 0)
			ext = ".so";
		
		while (fImpl->Next(outUrl, ext))
		{
			if (wantText && not IsTextFile(outUrl))
				continue;
			return true;
		}
	}

	return false;
}

#pragma mark -

int CreateTempFile(const HUrl& inDirectory, const string& inBaseName,
	HUrl& outURL)
{
	string path = inDirectory.GetPath() + '/' + inBaseName + ".XXXXXXXX";
	
	int fd = ::mkstemp(const_cast<char*>(path.c_str()));
	ThrowIfPOSIXErr(fd);
	
	outURL.SetScheme(kFileScheme);
	outURL.SetNativePath(path);
	
	return fd;
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

void SafeSaver::Commit()
{
	HUrl tmp(fSavedFile);
	tmp.SetFileName(tmp.GetFileName() + "~~");

	if (Exists(fSavedFile))
	{
		if (Rename(fSavedFile, tmp) < 0)
			THROW(("Could not rename old file"));
	}
	
	if (Rename(fTempFile, fSavedFile) < 0)
		THROW(("Could not rename new file"));
	
	Unlink(tmp);

	fCommited = true;
}

#pragma mark -

int Open(const HUrl& inURL, int inFlags)
{
	int fd;
	string path = inURL.GetPath();
	fd = ::open(path.c_str(), inFlags, 0644);
	if (fd < 0)
		HErrno = errno;
	return fd;
}

void Close(int inFD)
{
	::close(inFD);
}

bool Exists(const HUrl& inURL)
{
	string path = inURL.GetPath();

	struct stat st;
	return ::stat(path.c_str(), &st) == 0;
}

bool IsSameFile(const HUrl& inURL1, const HUrl& inURL2)
{
	struct stat st1, st2;

	int r1 = ::stat(inURL1.GetPath().c_str(), &st1);
	int r2 = ::stat(inURL2.GetPath().c_str(), &st2);
	
	return
		r1 == r2 &&
		r1 >= 0 &&
		st1.st_dev == st2.st_dev &&
		st1.st_ino == st2.st_ino;
}

char* GetCWD(char* outCwd, int inBufferSize)
{
	return getcwd(outCwd, inBufferSize);
}

int MkDir(const HUrl& inUrl)
{
	string path = inUrl.GetPath();
	int result = ::mkdir(path.c_str(), 0777);
	HErrno = errno;
	return result;
}

static void CopyFork(int fd1, int fd2)
{
	const unsigned long kBufferSize = 10240;
	HAutoBuf<char> tmp(new char[kBufferSize]);
	char* b = tmp.get();

	unsigned long size = Seek(fd1, 0, SEEK_END);
	Seek(fd1, 0, SEEK_SET);
	Seek(fd2, 0, SEEK_SET);
	
	while (size > 0)
	{
		unsigned long l = size;
		if (l > kBufferSize) l = kBufferSize;
		
		Read(fd1, b, l);
		Write(fd2, b, l);
		
		size -= l;
	}
}

void Copy(const HUrl& inFromURL, const HUrl& inToURL)
{
	int fd1 = -1, fd2 = -1;
	
	try
	{
		ThrowIfPOSIXErr(fd1 = Open(inFromURL, O_RDONLY));
		ThrowIfPOSIXErr(fd2 = Open(inToURL, O_RDWR | O_CREAT | O_TRUNC));
		
		CopyFork(fd1, fd2);
		
		Close(fd1);
		Close(fd2);
	}
	catch (...)
	{
		if (fd1 >= 0 -1) Close(fd1);
		if (fd2 >= 0 -1) Close(fd2);
		throw;
	}
}

int CopyAttributes(const HUrl& inFromURL, const HUrl& inToURL, bool inStripCKID)
{//
//	int fd1 = -1, fd2 = -1, result = 0;
//	
//	try
//	{
//		ThrowIfPOSIXErr(fd1 = Open(inFromURL, O_RDONLY | O_RSRC));
//		ThrowIfPOSIXErr(fd2 = Open(inToURL, O_RDWR | O_CREAT | O_TRUNC | O_RSRC));
//		
//		CopyFork(fd1, fd2);
//		
//		Close(fd1);
//		Close(fd2);
//		
//		if (inStripCKID)
//		{
//			FSSpec toSpec;
//			inToURL.GetSpecifier(toSpec);
//			
//			fd2 = ::FSpOpenResFile(&toSpec, fsRdWrPerm);
//			if (::ResError() == kNoError)
//			{
//				::UseResFile(fd2);
//				Handle h = ::Get1Resource('ckid', 128);
//				if (h) ::RemoveResource(h);
//				::CloseResFile(fd2);
//			}
//		}
//	}
//	catch (...)
//	{
//		if (fd1 >= 0 -1) Close(fd1);
//		if (fd2 >= 0 -1) Close(fd2);
//		result = -1;
//	}
//	
//	return result;
	return 0;
}

int64 Seek(int inFD, int64 inOffset, int inMode)
{
#if P_DEBUG
	int64 err = ::lseek(inFD, inOffset, inMode);

	if (err < 0) {
		PRINT(("inOffset: %Ld\n", inOffset));
		PRINT(("err: %d (%s)\n", errno, strerror(errno)));
	}
	return err;
#else
	return ::lseek(inFD, inOffset, inMode);
#endif
}

int32 Read(int inFD, void* inBuffer, uint32 inSize)
{
	int result;
	
	for (;;)
	{
		result = ::read(inFD, inBuffer, inSize);
		if (result >= 0 or errno != EINTR)
			break;
	}

	HErrno = errno;
	return result;
}

int32 PRead(int inFD, void* inBuffer, uint32 inSize, int64 inOffset)
{
	int result;
	
	for (;;)
	{
		result = ::pread(inFD, inBuffer, inSize, inOffset);
		if (result >= 0 or errno != EINTR)
			break;
	}
		
	HErrno = errno;
	return result;
}

int32 Write(int inFD, const void* inBuffer, uint32 inSize)
{
	int result;
	
	for (;;)
	{
		result = ::write(inFD, inBuffer, inSize);
		if (result >= 0 or errno != EINTR)
			break;
	}

	HErrno = errno;
	return result;
}

int32 PWrite(int inFD, const void* inBuffer, uint32 inSize, int64 inOffset)
{
	int result;
	
	for (;;)
	{
		result = ::pwrite(inFD, inBuffer, inSize, inOffset);
		if (result >= 0 or errno != EINTR)
			break;
	}

	HErrno = errno;
	return result;
}

void Truncate(int inFD, int64 inLength)
{
	(void)::ftruncate(inFD, inLength);
	HErrno = errno;
//	return result;
}

int Unlink(const HUrl& inURL)
{
	string path = inURL.GetPath();
	return ::unlink(path.c_str());
}

int Rename(const HUrl& inFromURL, const HUrl& inToURL)
{
	int result = -1;

	string oldPath, newPath;
		
	oldPath = inFromURL.GetPath();
	newPath = inToURL.GetPath();
	
	result = rename(oldPath.c_str(), newPath.c_str());
	HErrno = errno;
	return result;
}

int Sync(int inFD)
{
	int result = ::fsync(inFD);
	HErrno = errno;
	return result;
}

int GetCreationTime(const HUrl& inURL, int64& outTime)
{
	struct stat st;
	string path = inURL.GetPath();
	
	int r = ::stat(path.c_str(), &st);
	if (r == 0)
		outTime = st.st_ctime;

	return r;
}

int GetModificationTime(const HUrl& inURL, long long& outTime)
{
	struct stat st;
	string path = inURL.GetPath();
	
	int r = ::stat(path.c_str(), &st);
	if (r == 0)
		outTime = st.st_mtime;

	return r;
}

int SetModificationTime(const HUrl& inURL, long long inTime)
{
	return -1;
}

bool IsReadOnly(const HUrl& inURL)
{
	bool readOnly = false;

	if (inURL.IsLocal())
	{	
		struct stat st;
		string path = inURL.GetNativePath();
			
		if (stat(path.c_str(), &st) == 0)
		{
			readOnly = not ((gUid == st.st_uid && (S_IWUSR & st.st_mode)) ||
							(gGid == st.st_gid && (S_IWGRP & st.st_mode)) ||
							(S_IWOTH & st.st_mode));
			
			if (readOnly && S_IWGRP & st.st_mode)
			{
				int ngroups = getgroups(0, nil);
				if (ngroups > 0)
				{
					HAutoBuf<gid_t> groups(new gid_t[ngroups]);
					(void)getgroups(ngroups, groups.get());
					
					for (int i = 0; readOnly && i < ngroups; ++i)
					{
						if (groups.get()[i] == st.st_gid)
							readOnly = false;
					}
				}
			}
		}
	}
	
	return readOnly;
}

void SetReadOnly(const HUrl& inURL, bool inReadOnly)
{
	if (inURL.IsLocal())
	{	
		struct stat st;
		string path = inURL.GetNativePath();
			
		if (stat(path.c_str(), &st) == 0)
		{
			if (inReadOnly)
				st.st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
			else
			{
				long t = umask(0);	// get the umask
				umask(t);			// and reset it.
				st.st_mode |= (S_IWUSR | S_IWGRP | S_IWOTH) & ~t;
			}
				
			(void)chmod(path.c_str(), st.st_mode);
		}
	}
}

int OpenAttributes(const HUrl& inURL, int inPermissions)
{
	HUrl dir = inURL.GetParent().GetChild(".pepperstate");
	if (not Exists(dir) && MkDir(dir) == -1)
		return -1;
	
	return Open(dir.GetChild(inURL.GetFileName()), inPermissions);
}

void CloseAttributes(int inFD)
{
	Close(inFD);
}

int ReadAttribute(int inFD, unsigned long inType,
		int inID, void* outData, int inDataSize)
{
	int result = -1;
	if (inType == 'Pprs' && inID == 129)
	{
		Seek(inFD, 0, SEEK_SET);
		result = Read(inFD, outData, inDataSize);
	}
	return result;
}

int WriteAttribute(int inFD, unsigned long inType,
		int inID, const void* inData, int inDataSize)
{
	int result = -1;
	if (inType == 'Pprs' && inID == 129)
	{
		Seek(inFD, 0, SEEK_SET);
		result = Write(inFD, inData, inDataSize);
	}
	return result;
}

int GetAttributeSize(int inFD, unsigned long inType, int inID)
{
	return -1;
}

int ReadAttribute(int inFD, unsigned long inType,
		const char* inName, void* outData, int inDataSize)
{
	return -1;
}

int WriteAttribute(int inFD, unsigned long inType,
		const char* inName, const void* inData, int inDataSize)
{
	return -1;
}

int GetAttributeSize(int inFD, unsigned long inType, const char* inName)
{
	return -1;
}

void SetMimeType(const HUrl& inURL, const char* inMimeType)
{
}

void GetMimeType(const HUrl& inURL, char* outMimeType, int inMaxSize)
{
}

bool IsTextFile(const HUrl& inUrl)
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

bool IsLink(const HUrl& inUrl)
{
	struct stat st;
	string path = inUrl.GetPath();
	
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISLNK(st.st_mode);
}

bool IsDirectory(const HUrl& inUrl)
{
	struct stat st;
	string path = inUrl.GetPath();
	
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

bool IsExecutable(const HUrl& inURL)
{
	bool executable = false;
	
	if (inURL.IsLocal() && inURL.IsValid())
	{
		string path = inURL.GetNativePath();
		
		struct stat st;
		
		if (stat(path.c_str(), &st) == kNoError)
		{
			executable = ((gUid == st.st_uid && (S_IXUSR & st.st_mode)) ||
						  (gGid == st.st_gid && (S_IXGRP & st.st_mode)) ||
						  (S_IXOTH & st.st_mode));
		}
	}

	return executable;
}

void SetExecutable(const HUrl& inURL, bool inExecutable)
{
	if (inURL.IsLocal() && inURL.IsValid())
	{
		string path = inURL.GetNativePath();

		struct stat st;
		
		if (stat(path.c_str(), &st) == 0)
		{
			if (inExecutable)
			{
				long t = umask(0);	// get the umask
				umask(t);			// and reset it.
				st.st_mode |= (S_IXUSR | S_IXGRP | S_IXOTH) & ~t;
			}
			else
				st.st_mode &= ~(S_IXUSR | S_IXGRP | S_IXOTH);
			
			chmod(path.c_str(), st.st_mode);
		}
	}
}

}

// ------------------------------------------------------

#include <sys/mman.h>
#include <iostream>

//#include <boost/iostreams/device/mapped_file.hpp>

//struct HMMappedFileStreamImp : public boost::iostreams::mapped_file
//{
//};

struct HMMappedFileStreamImp
{
	void*		fBasePtr;
};

HMMappedFileStream::HMMappedFileStream(HFileStream& inFile, int64 inOffset, int64 inLength)
	: fImpl(new HMMappedFileStreamImp)
{
	if (inOffset + inLength > inFile.fSize)
		THROW(("Error creating memmory mapped file stream"));
		
	int64 pageSize = ::getpagesize();
	int64 offset = pageSize * (inOffset / pageSize);
	uint32 before = inOffset - offset;

	fImpl->fBasePtr = ::mmap(0, inLength + before, PROT_READ, MAP_PRIVATE, inFile.fFD, offset);

	if (reinterpret_cast<int64>(fImpl->fBasePtr) == -1)
		THROW(("Memmory mapping failed: %s\n", strerror(errno)));
	
	delete[] fData;
	
	fData = static_cast<char*>(fImpl->fBasePtr) + before;
	fLogicalSize = inLength;
	fPhysicalSize = inLength + before;
	fPointer = 0;
	fReadOnly = true;
	
	madvise(fImpl->fBasePtr, inLength + before, MADV_SEQUENTIAL);
}

HMMappedFileStream::~HMMappedFileStream()
{
	::munmap(fImpl->fBasePtr, fLogicalSize);
	delete fImpl;
}

