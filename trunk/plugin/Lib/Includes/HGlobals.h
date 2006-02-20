/*	$Id$
	Copyright Hekkelman Programmatuur b.v.
	Created Thursday September 06 2001 10:26:17
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
 
#ifndef HGLOBALS_H
#define HGLOBALS_H

#include "HUrl.h"
#include "HTypes.h"

extern long 				gOSVersion;
extern unsigned short		gUid;
extern unsigned short		gGid;
extern const unsigned long	kCreatorCode;
extern const unsigned long	kDefaultDocumentType;
extern const char			kAppName[], kVersionString[];
extern HUrl					gTempURL, gPrefsURL, gAppURL, gUserURL;
extern bool					gAntiAliasedText;

extern const std::string	kEmptyString;

extern const char			kHexChars[];

/*
namespace HBSD {
extern const socket_t		kInvalidSocket;
}
*/

/*
	These are the colors used in Pepper, but they are usefull
	for other apps as well.
*/
enum {
	kLowColor,				// background color
	kSelectionColor,		// background color of selected text
	kInvisiblesColor,		// color for control/space characters
	kMarkColor,				// color for the emacs mark
	kFontColor,				// the regular font color
	kLineNumberColor,		// color for the line numbers
	kCurLineColor,			// background color for the current line
	kRawTextColor,			// color to use for input methods, underline color
	kConvertedTextColor,	// same, but now for text converted by the input method

	kActiveDocBackColor,	// for themes drawing
	kInactiveDocBackColor,
	kActiveDlogBackColor,
	kInactiveDlogBackColor,
	
	kActiveDlogTextColor,
	kInactiveDlogTextColor,
	
	kActiveLineColor,
	kInactiveLineColor,
	
	kOSSelectionColor,		// for the windows selection color
	kOSSelectionTextColor,
	kOSInactiveSelectionColor,
	kOSInactiveSelectionTextColor,

	kLastColor				// used for counting the colors
};

#ifndef MINI_H_LIB
extern HColor gColor[kLastColor];
#endif

void InitHGlobals();

#endif // HGLOBALS_H
