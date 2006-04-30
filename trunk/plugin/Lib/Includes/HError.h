/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Wednesday August 22 2001 15:01:20
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
 
#ifndef HERROR_H
#define HERROR_H

#include <exception>
#include "HTypes.h"

/*
	Error code
*/
enum {
	kNoError				= 0,
	
#ifndef MINI_H_LIB

	pError					= 200,
	pErrIO,
	pErrResTooShort,
	pErrResReadOnly,
	pErrNilPointer,
	pErrPrefName,
	pErrPosix,
	pErrStatus,
	pErrLogic,
	pErrCannotRename,
	pErrFileCorrupt,
	pXError,
	pErrLocale,
	pErrNeedRGB,
	pErrColormap,
	pErrDragNotAccepted,
	pErrFreeType,
	pErrInvalidButtonBar,
	pErrExtTooLong,
	pErrCannotAssocExt,
	pErrX,
	pErrCouldNotOpenConnection,
	pErrSftpProtocolVersion,
	pErrAuthenticationProtocol,
	pErrCertificateNotFound,
	pErrMoveBackupFailed,

#endif
	
	pLastHError,
	pWinError
};

class HError : public std::exception
{
  public:
#ifndef MINI_H_LIB
				HError(HErrorCode inErr, int inInfo = 0);
				HError(HErrorCode inErr, const char* inInfo);
#endif
				HError(const char* inMessage, ...);
#if P_WIN
				HError(unsigned long inErrCode, bool, bool);
#endif

	const char*	GetErrorString() const;
	HErrorCode	GetErrorCode() const;

	virtual const char* what() const throw();

  private:
	char		fMessage[256];
	HErrorCode	fErrCode;
	long		fInfo;
};

inline const char* HError::GetErrorString() const
{
	return fMessage;
}

inline HErrorCode HError::GetErrorCode() const
{
	return fErrCode;
}

#if P_DEBUG

extern bool		gOKToThrow;
void			ReportThrow(const char* inFunction, const char* inFile, int inLine);

#if P_MAC || P_UNIX
	#if (P_CODEWARRIOR && __MWERKS__ < 0x2300) || P_IRIX
		extern const char __func__[];
	#endif
	#define	THROW(a)do { \
						if (not gOKToThrow) ReportThrow(__func__, __FILE__, __LINE__); \
							throw HError a; \
					} while (false)
#else

	#define	THROW(a)	do { \
						if (not gOKToThrow) ReportThrow("Unknown", __FILE__, __LINE__); \
							throw HError a; \
					} while (false)
#endif /* P_MAC */

class StOKToThrow
{
  public:
	StOKToThrow();
	~StOKToThrow();
  private:
	bool	fWasOK;
};

#else /* ! P_DEBUG */

#define THROW(a)	throw HError a

class StOKToThrow
{
  public:
	StOKToThrow()	{}
	~StOKToThrow()	{}
};

#endif

#ifdef MINI_H_LIB
#	define ThrowIfNil(p)			do { const void* __p = (p); if (__p == nil) THROW(("Nil pointer")); } while (false)
#	define ThrowIfPOSIXErr(e)		do { if ((e) < 0) THROW(("Posix error: %s", std::strerror(static_cast<unsigned long>(HFile::HErrno)), true, true)); } while (false)
#if P_WIN
#	define ThrowIfOSErr(e)			do { unsigned long __e = static_cast<unsigned long>(e); if (__e != 0) THROW((__e, true, true)); } while (false)
#	define ThrowIfFalse(b)			do { if (not (b)) THROW((::GetLastError(), true, true)); } while (false)
#endif
#	define ThrowIf(b)				do { if ((b)) THROW(("Logic error")); } while (false)
#else
#	define ThrowIfNil(p)			do { const void* __p = (p); if (__p == nil) THROW((pErrNilPointer)); } while (false)
#	if P_WIN
#		define ThrowIfPOSIXErr(e)	do { if ((e) < 0) THROW((static_cast<unsigned long>(HFile::HErrno), true, true)); } while (false)
#	else
#		define ThrowIfPOSIXErr(e)	do { if ((e) < 0) THROW((pErrPosix, (long)HFile::HErrno)); } while (false)
#	endif
#	define ThrowIf(b)				do { if ((b)) THROW((pErrLogic, 0)); } while (false)

#	if P_MAC
#		define ThrowIfOSStatus(e)	do { OSStatus __e = (e); if (__e != 0) THROW((pErrStatus, __e)); } while (false)
#	elif P_UNIX
#		define ThrowIfXError(e)		do { int __e = (e); if (__e != Success) THROW((pXError, __e)); } while (false)
#	elif P_WIN
#		define ThrowIfOSErr(e)		do { unsigned long __e = static_cast<unsigned long>(e); if (__e != 0) THROW((__e, true, true)); } while (false)
#		define ThrowIfFalse(b)		do { if (not (b)) THROW((::GetLastError(), true, true)); } while (false)
#	endif
#endif

#if P_DEBUG
	extern void debug_printf(const char*, ...);
	extern void set_debug_info (const char* inFilename, int inLineNumber);
#	define PRINT(a)			do { set_debug_info (__FILE__, __LINE__); debug_printf a; } while (false)
#else
#	define PRINT(a)			do {} while (false)
#endif

void DisplayError(const std::exception& e);

#endif // HERROR_H
