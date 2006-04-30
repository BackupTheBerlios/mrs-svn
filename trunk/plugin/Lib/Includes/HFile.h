/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Tuesday September 12 2000 08:53:39
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
 
#ifndef HFILE_H
#define HFILE_H

#include "HUrl.h"
#include <vector>
#include <fcntl.h>
#include "HGlobals.h"

#ifndef O_BINARY
#define O_BINARY	0
#endif

// HFileExt is a wrapper that helps finding out whether
// an url matches a set of filename patterns.
// The strings can also be used to fill a standard Open dialog
// on Windows

class HFileExt
{
  public:
				HFileExt();
				HFileExt(const HFileExt& inOther);
				HFileExt(std::string inDesc, std::string inExt,
					std::string inDefaultExtension = kEmptyString);
	
	void		SetDescription(std::string inDesc);
	std::string	GetDescription() const;

	void		SetExtensions(std::string inExt);
	std::string	GetExtensions() const;

	bool		Matches(const HUrl& inUrl) const;
	bool		Matches(std::string inName) const;
	
	std::string	GetDefaultExtension() const;
	
					// to keep a set unique
	bool		operator==(const HFileExt& inOther) const;
	
  private:
	std::string	fDesc;
	std::string	fExt;
	std::string fDefaultExtension;
};

namespace HFile
{
		// should find a way to move this one elsewhere
	class HExtensionSet : public std::vector<HFileExt>
	{
	  public:
		static HExtensionSet&	Instance();
		
					HExtensionSet();
	
		void		Init();
		void		GetFilter(std::string& outFilter);
		int			GetIndex(std::string inName) const;
		
		int			Count() const;
		HFileExt	operator[] (int inIndex) const;
	};

		// our errno, use as the usual POSIX errno
		// used to be a global h_errno but that conflicts with a DEFINE
		// Winsock.h... sigh
		// And why the fuck is errno defined too?
	extern int HErrno;

	class FileIterator
	{
	  public:
					FileIterator(const HUrl& inDirURL, bool inRecursive);
					~FileIterator();

						// returns filtered by mime type
		bool		Next(HUrl& outURL, const char* inMimeType = nil);
		
						// returns next text file
		bool		NextText(HUrl& outURL);

	  private:
							FileIterator(const FileIterator&);
		FileIterator&		operator=(const FileIterator&);

		struct IteratorImp*	fImpl;
	};
	
#ifndef MINI_H_LIB
	class GlobIterator
	{
	  public:
						GlobIterator(std::string inGlobPattern);
		virtual			~GlobIterator();

		bool			Next(HUrl& outUrl);

	  private:
						GlobIterator(const GlobIterator&);
		GlobIterator&	operator=(const GlobIterator&);
	  
		struct GlobImp*	fImpl;
	};
#endif
	
	class Chooser
	{
	  public:
					Chooser();
					~Chooser();
		
		void		SetTitle(std::string inTitle);
		void		SetAllowOpenReadOnly(bool inAllow);
		
		bool		AskChooseOneFile(HUrl& outURL, const char* inMimeType = nil);
		bool		AskChooseOneObject(HUrl& outURL);
		bool		AskChooseFiles(const char* inMimeType = nil);
		bool		AskChooseFiles(long inNumTypes, unsigned long* inTypes);
		bool		AskChooseFolder(HUrl& outURL);
		
		bool		Next(HUrl& outURL);
		bool		OpenReadOnly();

	  private:
		struct ChooserImp* fImpl;
	};

	class Designator
	{
	  public:
					Designator();
					~Designator();
		
		bool		AskDesignateFile(std::string inName, const char* inMimeType,
						HUrl& outUrl);
		bool		IsReplacing() const;

	  private:
		struct DesignatorImp* fImpl;
	};
		
	class SafeSaver
	{
	  public:
					SafeSaver(const HUrl& inSpec);
					~SafeSaver();
	
		const HUrl	GetURL() const			{ return fTempFile; }
		
		void		Commit();
	
	  private:
		HUrl		fSavedFile;
		HUrl		fTempFile;
		bool		fCommited;
	};
		
	int				Open(const HUrl& inURL, int inFlags);
	void			Close(int inFD);
	
	void			Pipe(int& outReadFD, int& outWriteFD);
	
	bool			Exists(const HUrl& inURL);
	bool			IsSameFile(const HUrl& inURL1, const HUrl& inURL2);
	
	int				MkDir(const HUrl& inURL);
	char*			GetCWD(char* inBuffer, int inBufferLength);
	
	void			Copy(const HUrl& inFromURL, const HUrl& inToURL);
	
	int64			Seek(int inFD, int64 inOffset, int inMode);
	int64			Tell(int inFD);

	int32			Write(int inFD, const void* inData, uint32 inSize);
	int32			PWrite(int inFD, const void* inData, uint32 inSize, int64 inOffset);

	int32			Read(int inFD, void* inData, uint32 inSize);
	int32			PRead(int inFD, void* inData, uint32 inSize, int64 inOffset);

	void			Truncate(int inFD, int64 inSize);

					// CreateTempFile creates a unique file and returns a file descriptor to it
					// So it is open, no need to open it first.
	int				CreateTempFile(const HUrl& inDirectory, const std::string& inBaseName,
						HUrl& outURL);

//						// and for MacOS X we have to return a native path...
//	void			MakeTempFile(std::string& outPath, const char* inBaseName);
	
	int				Unlink(const HUrl& inURL);
	int				Rename(const HUrl& inFromURL, const HUrl& inToURL);

	int				Sync(int inFD);

	int				GetCreationTime(const HUrl& inURL, int64& outTime);
	int				GetModificationTime(const HUrl& inURL, int64& outTime);
	int				SetModificationTime(const HUrl& inURL, int64 inTime);
	
	bool			IsReadOnly(const HUrl& inURL);
	void			SetReadOnly(const HUrl& inURL, bool inReadOnly);

	int				CopyAttributes(const HUrl& inFromURL, const HUrl& inToURL, bool inStripCKID);

	// to read attributes from a file, open it first
	int				OpenAttributes(const HUrl& inURL, int inPermissions);
	void			CloseAttributes(int inFD);

	// by name
	int				ReadAttribute(int inFD, unsigned long inType,
						const char* inAttrName, void* outData, int inDataSize);
	int				WriteAttribute(int inFD, unsigned long inType,
						const char* inAttrName, const void* inData, int inDataSize);
	int				GetAttributeSize(int inFD, unsigned long inType, const char* inName);

	// by id
	int				ReadAttribute(int inFD, unsigned long inType, int inID,
						void* outData, int inDataSize);
	int				WriteAttribute(int inFD, unsigned long inType, int inID,
						const void* inData, int inDataSize);
	int				GetAttributeSize(int inFD, unsigned long inType, int inID);

	void			SetMimeType(const HUrl& inURL, const char* inMimeType);
	void			GetMimeType(const HUrl& inURL, char* outMimeType, int inMaxSize);
	bool			IsTextFile(const HUrl& inURL);
	bool			IsDirectory(const HUrl& inUrl);
	bool			IsLink(const HUrl& inUrl);

	void			AddTextFileNameExtension(const HFileExt& inExt);
	void			ClearTextFileNameExtensionMapping();
	
	bool			IsExecutable(const HUrl& inURL);
	void			SetExecutable(const HUrl& inURL, bool inExecutable);
}

#endif // HFILE_H
