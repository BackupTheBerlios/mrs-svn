/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Thursday August 17 2000 15:18:14
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
 
#ifndef HUrl_H
#define HUrl_H

#include <string>
#include "HNativeTypes.h"		// for HFileSpec

extern const char
	kFileScheme[],
	kFtpScheme[],
	kSftpScheme[],
	kHttpScheme[],
	kRegistryScheme[];

class HFont;

class HUrl
{
  public:
							HUrl();
							HUrl(const HUrl& inURL);
	explicit				HUrl(std::string inURL);
	explicit				HUrl(const HFileSpec& inSpec);
							~HUrl();

	HUrl					GetParent() const;
	HUrl					GetChild(std::string inFileName) const;

	bool					operator==(const HUrl& inURL) const;
	bool					operator==(std::string inURL) const;
	HUrl&					operator=(const HUrl& inURL);
	
	bool					IsLocal() const;
	bool					IsValid() const;
	bool					IsDirectory() const;
	bool					IsWindowsShare() const;
	
	void					NormalizePath();

	std::string				GetURL() const;
	void					SetURL(std::string inURL);

	std::string				GetScheme() const;
	void					SetScheme(std::string inScheme);
	
	HErrorCode				GetSpecifier(HFileSpec& outSpecifier) const;
	void					SetSpecifier(const HFileSpec& inSpecifier);
	
	std::string				GetFileName() const;
	void					SetFileName(std::string inName);
	
	std::string				GetPath() const;
	void					SetPath(std::string inPath);
	
	std::string				GetNativePath() const;
	void					SetNativePath(std::string inPath);
	
	void					GetTruncatedURL(std::string& outPath,
								const HFont& inFont, int inMaxWidth) const;

	std::string				GetHost() const;
	void					SetHost(std::string inHost);
	
	unsigned short			GetPort() const;
	void					SetPort(unsigned short inPort);
	
	std::string				GetUserName() const;
	void					SetUserName(std::string inUser);
	
	std::string				GetPassword() const;
	void					SetPassword(std::string inPassword);

	static void				EncodeReservedChars(std::string& ioPath);
	static void				DecodeReservedChars(std::string& ioPath);

  private:
	
	void					Decode(std::string& outScheme, std::string& outHost,
								std::string& outUser, std::string& outPassword,
								unsigned short& outPort, std::string& outPath) const;
	void					EncodeFtp(std::string inHost, std::string inUser,
								std::string inPassword, unsigned short inPort,
								std::string inPath);
	void					EncodeSftp(std::string inHost, std::string inUser,
								std::string inPassword, unsigned short inPort,
								std::string inPath);
	void					EncodeFile(std::string inScheme, std::string inPath);
//	void					EncodeAlias(const HFileSpec& inFile);
//	void					EncodeHFileSpec(const HFileSpec& inFile);
//	void					EncodeHFS(const HFileSpec& inFile);

	std::string				fData;
	mutable HFileSpec		fSpec;
};	

#endif // HUrl_H
