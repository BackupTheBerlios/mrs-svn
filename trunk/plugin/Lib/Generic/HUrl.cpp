/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Thursday August 17 2000 15:17:58
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

#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "HFile.h"
#include "HError.h"
//#include "HFont.h"
#include "HUtils.h"
#include "HGlobals.h"

static HFileSpec sNullSpec;

#ifndef HURL_WITH_TRUNCATE
#define HURL_WITH_TRUNCATE 0
#endif

using namespace std;

const char
	kFileScheme[] = "file:",
	kFtpScheme[] = "ftp:",
	kSftpScheme[] = "sftp:",
	kHttpScheme[] = "http:",
	kRegistryScheme[] = "registry:";

inline char ConvertHex(char inChar)
{
	int	value = inChar - '0';

	if (inChar >= 'A'  &&  inChar <= 'F') {
		value = inChar - 'A' + 10;

	} else if (inChar >= 'a'  &&  inChar <= 'f') {
		value = inChar - 'a' + 10;
	}

	return (char) value;
}

unsigned char kURLAcceptable[96] =
{/* 0 1 2 3 4 5 6 7 8 9 A B C D E F */
    0,0,0,0,0,0,0,0,0,0,7,6,0,7,7,4,		/* 2x   !"#$%&'()*+,-./	 */
    7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,		/* 3x  0123456789:;<=>?	 */
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 4x  @ABCDEFGHIJKLMNO  */
    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,		/* 5X  PQRSTUVWXYZ[\]^_	 */
    0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,		/* 6x  `abcdefghijklmno	 */
    7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0			/* 7X  pqrstuvwxyz{\}~	DEL */
};

void HUrl::EncodeReservedChars(string &ioPath)
{
	string path(ioPath);
	
	ioPath.erase(ioPath.begin(), ioPath.end());
	
	for (unsigned int i = 0; i < path.length(); ++i)
	{
		unsigned char a = (unsigned char) path[i];
		if (!(a >= 32 && a < 128 && ((kURLAcceptable[a-32]) & 4)))
		{
			ioPath += '%';
			ioPath += kHexChars[a >> 4];
			ioPath += kHexChars[a & 15];
		}
		else
			ioPath += path[i];
	}
}

void HUrl::DecodeReservedChars(string& ioPath)
{
	string::iterator p;
	
	HAutoBuf<char> path(new char[ioPath.length() + 1]);
	char* r = path.get();
	
	for (p = ioPath.begin(); p != ioPath.end(); ++p)
	{
		char q = *p;

		if (q == '%' && ++p != ioPath.end())
		{
			q = (char) (ConvertHex(*p) * 16);

			if (++p != ioPath.end())
				q = (char) (q + ConvertHex(*p));
		}

		*r++ = q;
	}
	
	ioPath.assign(path.get(), r);
}

#pragma mark -

HUrl::HUrl()
	: fData(":")
	, fSpec(sNullSpec)
{
}

HUrl::HUrl(const HUrl& inURL)
	: fData(inURL.fData)
	, fSpec(inURL.fSpec)
{
}

HUrl::HUrl(string inURL)
//	: fData(inURL)
//	, fSpec(sNullSpec)
{
	fData = inURL;
	fSpec = sNullSpec;

	if (fData.length() > 0 && inURL[0] == '/')
	{
		SetScheme(kFileScheme);
		SetPath(inURL);
	}
	else
	{
		string scheme, host, user, password, path;
		unsigned short port;
		
		Decode(scheme, host, user, password, port, path);
		if (scheme.compare(kFileScheme) == 0 ||
			scheme.compare(kRegistryScheme) == 0)
		{
			EncodeFile(scheme, path);
		}
		else if (scheme.compare(kFtpScheme) == 0)
			EncodeFtp(host, user, password, port, path);
		else if (scheme.compare(kSftpScheme) == 0)
			EncodeSftp(host, user, password, port, path);
	
		fSpec = sNullSpec;
	}
}

HUrl::HUrl(const HFileSpec& inSpecifier)
{
	SetSpecifier(inSpecifier);
}

HUrl::~HUrl()
{
}

HUrl HUrl::GetParent() const
{
	HUrl url;
	bool gotIt = false;

	url.SetScheme(GetScheme());

	if (HFileSpec::valid && not (fSpec == sNullSpec) && fSpec.IsValid())
	{
		HFileSpec parent = fSpec.GetParent();
		url.SetSpecifier(parent);
		gotIt = true;
	}
	
	if (not gotIt)
	{
		string path = GetPath();
		string::size_type d = path.rfind('/');
		if (d == path.length() - 1)
		{
			path.erase(path.length() - 1, 1);
			d = path.rfind('/');
		}
		if (d == string::npos || d == 0)
			path = "/";
		else
			path.erase(d + 1, path.length() - d - 1);
		url.SetPath(path);
	}
	
	return url;
}

HUrl HUrl::GetChild(string inFileName) const
{
	if (HFileSpec::valid && not (fSpec == sNullSpec))
	{
		HFileSpec child(fSpec, inFileName.c_str());
		return HUrl(child);
	}
	else
	{
		string url = GetURL();
		return HUrl(url + '/' + inFileName);
	}
}

bool HUrl::operator==(const HUrl& inURL) const
{
	if (HFileSpec::valid && inURL.IsLocal() && IsLocal())
	{
		HFileSpec mySpec, otherSpec;
		return
			GetSpecifier(mySpec) == kNoError &&
			inURL.GetSpecifier(otherSpec) == kNoError &&
			mySpec == otherSpec;
	}
	else if (fData == inURL.fData)
		return true;
	else if (IsLocal() && inURL.IsLocal())
		return HFile::IsSameFile(*this, inURL);
	else if (strcasecmp(GetScheme().c_str(), inURL.GetScheme().c_str()) == 0)
	{
		string p1, p2;

		p1 = GetPath();
		p2 = inURL.GetPath();
		
		if (p1[p1.length() - 1] == '/')	p1.erase(p1.length() - 1, 1);
		if (p2[p2.length() - 1] == '/') p2.erase(p2.length() - 1, 1);
		return p1 == p2;
	}
	else
		return false;
}

bool HUrl::operator==(string inURL) const
{
	return operator==(HUrl(inURL));
}

HUrl& HUrl::operator=(const HUrl& inURL)
{
	fData = inURL.fData;
	fSpec = inURL.fSpec;
	return *this;
}

bool HUrl::IsLocal() const
{
	if (HFileSpec::valid && not (fSpec == sNullSpec))
		return true;
	else
		return strcasecmp(GetScheme().c_str(), kFileScheme) == 0;
}

bool HUrl::IsValid() const
{
	string scheme = GetScheme();
	return
#if P_WIN
		strcasecmp(scheme.c_str(), kRegistryScheme) == 0 ||
#endif
		strcasecmp(scheme.c_str(), kFileScheme) == 0 ||
		strcasecmp(scheme.c_str(), kFtpScheme) == 0 ||
		strcasecmp(scheme.c_str(), kSftpScheme) == 0;
}

//bool HUrl::IsLink() const
//{
////	if (HFileSpec::valid)
////	{
////		if (fSpec == sNullSpec)
////		{
////			HFileSpec spec;
////			GetSpecifier(spec);
////		}
////		return fSpec.IsLink();
////	}
////	else if (IsLocal())
//		return HFile::IsLink(*this);
////	else
////		return false;
//}

bool HUrl::IsWindowsShare() const
{
	bool result = false;

#if P_WIN	
	if (IsLocal())
	{
		string p = GetNativePath();
		if (p.length() > 1 && p[0] == '\\' && p[1] == '\\')
			result = true;
	}
#endif

	return result;
}

bool HUrl::IsDirectory() const
{
	if (HFileSpec::valid)
	{
		if (fSpec == sNullSpec)
		{
			HFileSpec spec;
			GetSpecifier(spec);
		}
		return fSpec.IsDirectory();
	}
	else if (IsLocal())
		return HFile::IsDirectory(*this);
	else
		return fData[fData.length() - 1] == '/';
}

string HUrl::GetURL() const
{
	if (HFileSpec::valid &&
		(fData == "file://" || fData.length() == 0 || fData == ":") &&
		not (fSpec == sNullSpec))
	{
		string path;
		HUrl* self = const_cast<HUrl*>(this);

		try
		{
			fSpec.GetPath(path);

			EncodeReservedChars(path);

			self->fData = kFileScheme;
			self->fData += "//";
			self->fData += path;
		}
		catch (...)
		{
			self->EncodeFile(kFileScheme, kEmptyString);
		}
	}
	
	return fData;
}

void HUrl::SetURL(string inURL)
{
	fSpec = sNullSpec;
	fData = inURL;

	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	if (scheme.compare(kFileScheme) == 0 ||
		scheme.compare(kRegistryScheme) == 0)
	{
		EncodeFile(scheme, path);
	}
	else if (scheme.compare(kFtpScheme) == 0)
		EncodeFtp(host, user, password, port, path);
	else if (scheme.compare(kSftpScheme) == 0)
		EncodeSftp(host, user, password, port, path);
}

string HUrl::GetScheme() const
{
	string result;
	
	if (HFileSpec::valid && not (fSpec == sNullSpec))
		result = kFileScheme;
	else
	{
		string::size_type d = fData.find(':');
		if (d != string::npos)
			result = fData.substr(0, d + 1);
	}

	for (string::iterator p = result.begin(); p != result.end(); ++p)
		*p = static_cast<char>(tolower(*p));
	
	return result;
}

void HUrl::SetScheme(string inScheme)
{
	fSpec = sNullSpec;
	if (fData.length() < inScheme.length() ||
		strncasecmp(fData.c_str(), inScheme.c_str(), inScheme.length()))
	{
		string::size_type n = fData.find(':');
		if (n != string::npos)
		{
			fData.erase(0, n + 1);
			fData.insert(0, inScheme);
		}
		else
			fData = inScheme;
	}
}

HErrorCode HUrl::GetSpecifier(HFileSpec& outSpecifier) const
{
	if (HFileSpec::valid && IsLocal())
	{
		HErrorCode err = kNoError;
		
		if (fSpec == sNullSpec)
		{
			HFileSpec spec;
			
			err = spec.SetPath(GetPath());

			if (err == kNoError)
				fSpec = spec;
		}
			
		if (err == kNoError)
			outSpecifier = fSpec;

		return err;
	}
	else
#if MINI_H_LIB
		return -1;
#else
		return pErrLogic;
#endif
}

void HUrl::SetSpecifier(const HFileSpec& inSpecifier)
{
	fData = "file://";
	fSpec = inSpecifier;
}

string HUrl::GetFileName() const
{
	string path = GetPath();

	if (path[path.length() - 1] == '/')	// strip off dir separator if present
		path.erase(path.length() - 1, 1);
	string::size_type d = path.rfind('/');
	if (d != string::npos)
		path.erase(0, d + 1);
	return path;
}

void HUrl::SetFileName(string inName)
{
	string path = GetPath();

	fSpec = sNullSpec;
	string::size_type d = path.rfind('/');
	if (d != string::npos)
		path.erase(d + 1, path.length() - d - 1);
	path.append(inName);
	::NormalizePath(path);
	SetPath(path);
}

string HUrl::GetPath() const
{
	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	::NormalizePath(path);
	
	return path;
}

string HUrl::GetNativePath() const
{
	string result;

	if (HFileSpec::valid)
	{
		if (fSpec == sNullSpec)
			fSpec.SetPath(GetPath());
		fSpec.GetNativePath(result);
	}
	else
		result = GetPath();
	
	return result;
}

void HUrl::SetNativePath(string inPath)
{
	if (HFileSpec::valid)
	{
		HFileSpec spec;
		spec.SetNativePath(inPath);
		fSpec = spec;
		fData.erase(fData.begin(), fData.end());
	}
	else
		SetPath(inPath);
}

void HUrl::SetPath(string inPath)
{
	fSpec = sNullSpec;

	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	
	if (scheme == ":")
		scheme = kFileScheme;
	
	if (scheme.compare(kFileScheme) == 0 ||
		scheme.compare(kRegistryScheme) == 0)
	{
		EncodeFile(scheme, inPath);
	}
	else if (scheme.compare(kFtpScheme) == 0)
		EncodeFtp(host, user, password, port, inPath);
	else if (scheme.compare(kSftpScheme) == 0)
		EncodeSftp(host, user, password, port, inPath);
}

#if HURL_WITH_TRUNCATE

void HUrl::GetTruncatedURL(string& outURL, const HFont& inFont, int inMaxWidth) const
{
	string scheme = GetScheme();

//	if (strcmp(scheme, kFileScheme))
//	{
		scheme += "//";
		inMaxWidth -= inFont.StringWidth(scheme);
//	}
//	else
//		*scheme = 0;
	
	string path = GetPath();
	
	do
	{
		const char* pp = path.c_str();
		
		if (inFont.StringWidth(pp, static_cast<int>(path.length())) < inMaxWidth)
			break;
		
		char *p = strchr(pp, '/'), *r;
		r = p + 1;
		
		if (not p)
			break;
		
		int w = inFont.StringWidth("…") + inFont.StringWidth(p, r - pp);
	
		while (p && inFont.StringWidth(p, static_cast<int>(strlen(p))) + w >= inMaxWidth)
		{
			p = strchr(p + 1, '/');
		}
		
		if (p > r)
		{
			path.erase(static_cast<string::size_type>(r - pp),
				static_cast<string::size_type>(p - r));
			path.insert(static_cast<string::size_type>(r - pp), "…");
		}
	}
	while (P_NULL_VAR);

	if (scheme.length())
		outURL = scheme;
	else
		outURL = kEmptyString;
	outURL += path;
}

#endif

string HUrl::GetHost() const
{
	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	
	return host;
}

void HUrl::SetHost(string inHost)
{
	fSpec = sNullSpec;

	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	if (scheme.compare(kFtpScheme) == 0)
		EncodeFtp(inHost, user, password, port, path);
	else if (scheme.compare(kSftpScheme) == 0)
		EncodeSftp(inHost, user, password, port, path);
}

unsigned short HUrl::GetPort() const
{
	string scheme, host, user, password, path;
	unsigned short port = 0;
	
	Decode(scheme, host, user, password, port, path);
	
	return port;
}

string HUrl::GetUserName() const
{
	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	
	return user;
}

void HUrl::SetUserName(string inUser)
{
	fSpec = sNullSpec;

	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	if (scheme.compare(kFtpScheme) == 0)
		EncodeFtp(host, inUser, password, port, path);
	else if (scheme.compare(kSftpScheme) == 0)
		EncodeSftp(host, inUser, password, port, path);
}

string HUrl::GetPassword() const
{
	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	
	return password;
}

void HUrl::SetPassword(string inPassword)
{
	fSpec = sNullSpec;

	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	if (scheme.compare(kFtpScheme) == 0)
		EncodeFtp(host, user, inPassword, port, path);
	else if (scheme.compare(kSftpScheme) == 0)
		EncodeSftp(host, user, inPassword, port, path);
}

void HUrl::Decode(string& outScheme, string& outHost,
		string& outUser, string& outPassword,
		unsigned short& outPort, string& outPath) const
{
	(void)GetURL();	// be sure to init the fData...
	
	outScheme = GetScheme();
	outPort = 0;
	
	if (outScheme == kFileScheme && fData.compare(5, 2, "//") == 0)
	{
		// sometimes, there is a host ...
		string::size_type d = fData.find('/', 7);
		if (d == string::npos)
			d = 7;
		outPath.assign(fData, d, fData.length() - d);
		DecodeReservedChars(outPath);
	}
	else if (outScheme == kRegistryScheme && fData.compare(9, 2, "//") == 0)
	{
		outPath.assign(fData, 11, fData.length() - 11);
		DecodeReservedChars(outPath);
	}
	else if (outScheme == kFtpScheme && fData.compare(4, 2, "//") == 0)
	{
		string::size_type d;
		
		d = fData.find('/', 6);
		if (d != string::npos)
		{
			outPath.assign(fData, d + 1, fData.length() - d - 1);
			DecodeReservedChars(outPath);
		}
		else
			d = fData.length();
		
		string rest(fData, 6, d - 6);
		
		string::size_type u = rest.rfind('@');
		string::size_type p;
		
		if (u != string::npos)
		{
			outUser.assign(rest, 0, u);
			p = outUser.find(':');
			if (p != string::npos)
			{
				outPassword.assign(outUser, p + 1, outUser.length() - p - 1);
				outUser.erase(p, outUser.length() - p);
			}
			++u;
		}
		else
			u = 0;
		
		outHost.assign(rest, u, d - u);
		p = outHost.find(':');
		if (p != string::npos)
		{
			outPort = static_cast<unsigned short> (atoi(outHost.c_str() + p + 1));
			outHost.erase(p, outHost.length() - p);
		}
	}
	else if (outScheme == kSftpScheme && fData.compare(5, 2, "//") == 0)
	{
		string::size_type d;
		
		d = fData.find('/', 7);
		if (d != string::npos)
		{
			outPath.assign(fData, d + 1, fData.length() - d - 1);
			DecodeReservedChars(outPath);
		}
		else
			d = fData.length();
		
		string rest(fData, 7, d - 7);
		
		string::size_type u = rest.rfind('@');
		string::size_type p;
		
		if (u != string::npos)
		{
			outUser.assign(rest, 0, u);
			p = outUser.find(':');
			if (p != string::npos)
			{
				outPassword.assign(outUser, p + 1, outUser.length() - p - 1);
				outUser.erase(p, outUser.length() - p);
			}
			++u;
		}
		else
			u = 0;
		
		outHost.assign(rest, u, d - u);
		p = outHost.find(':');
		if (p != string::npos)
		{
			outPort = static_cast<unsigned short> (atoi(outHost.c_str() + p + 1));
			outHost.erase(p, outHost.length() - p);
		}
	}
}

void HUrl::EncodeFtp(string inHost, string inUser,
		string inPassword, unsigned short inPort,
		string inPath)
{
	fSpec = sNullSpec;

	fData = kFtpScheme;
	fData += "//";
	if (inUser.length())
	{
		fData += inUser;
		if (inPassword.length())
		{
			fData += ':';
			fData += inPassword;
		}
		fData += '@';
	}
	fData += inHost;
	if (inPort != 0)
	{
		fData += ':';
		fData += NumToString(inPort);
	}
	fData += '/';
	EncodeReservedChars(inPath);
	fData += inPath;
}

void HUrl::EncodeSftp(string inHost, string inUser,
		string inPassword, unsigned short inPort,
		string inPath)
{
	fSpec = sNullSpec;

	fData = kSftpScheme;
	fData += "//";
	if (inUser.length())
	{
		fData += inUser;
		if (inPassword.length())
		{
			fData += ':';
			fData += inPassword;
		}
		fData += '@';
	}
	fData += inHost;
	if (inPort != 0)
	{
		fData += ':';
		fData += NumToString(inPort);
	}
	fData += '/';
	EncodeReservedChars(inPath);
	fData += inPath;
}

void HUrl::EncodeFile(string inScheme, string inPath)
{
	fSpec = sNullSpec;

	fData = inScheme;
	fData += "//";
	::NormalizePath(inPath);
	EncodeReservedChars(inPath);
	fData += inPath;
}

void HUrl::NormalizePath()
{
	string scheme, host, user, password, path;
	unsigned short port;
	
	Decode(scheme, host, user, password, port, path);
	::NormalizePath(path);
	SetPath(path);
}
