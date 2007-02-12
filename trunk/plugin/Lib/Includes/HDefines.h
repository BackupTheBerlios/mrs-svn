/*
	HDefines
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
 
#ifndef HDEFINES_H
#define HDEFINES_H

enum {	// modifier keys
	kCmdKey						 = 1 << 0,
	kShiftKey 					 = 1 << 1,
	kOptionKey					 = 1 << 2,
	kControlKey					 = 1 << 3,
	kAlphaLock					 = 1 << 4,
	kNumPad 					 = 1 << 5,
	kRightShiftKey				 = 1 << 6,
	kRightOptionKey				 = 1 << 7,
	kRightControlKey			 = 1 << 8
};

enum {	// key codes
	kHNullKeyCode 				= 0,
	kHHomeKeyCode 				= 1,
	kHEnterKeyCode				= 3,
	kHEndKeyCode				= 4,
	kHHelpKeyCode 				= 5,
	kHBellKeyCode 				= 7,
	kHBackspaceKeyCode			= 8,
	kHTabKeyCode				= 9,
	kHLineFeedKeyCode 			= 10,
	kHVerticalTabKeyCode		= 11,
	kHPageUpKeyCode				= 11,
	kHFormFeedKeyCode 			= 12,
	kHPageDownKeyCode 			= 12,
	kHReturnKeyCode				= 13,
	kHFunctionKeyKeyCode		= 16,
	kHEscapeKeyCode				= 27,
	kHClearKeyCode				= 27,
	kHLeftArrowKeyCode			= 28,
	kHRightArrowKeyCode			= 29,
	kHUpArrowKeyCode			= 30,
	kHDownArrowKeyCode			= 31,
	kHSpaceKeyCode				= 32,
	kHDeleteKeyCode				= 127,
	kHF1KeyCode					= 0x0101,
	kHF2KeyCode					= 0x0102,
	kHF3KeyCode					= 0x0103,
	kHF4KeyCode					= 0x0104,
	kHF5KeyCode					= 0x0105,
	kHF6KeyCode					= 0x0106,
	kHF7KeyCode					= 0x0107,
	kHF8KeyCode					= 0x0108,
	kHF9KeyCode					= 0x0109,
	kHF10KeyCode				= 0x010a,
	kHF11KeyCode				= 0x010b,
	kHF12KeyCode				= 0x010c,
	kHF13KeyCode				= 0x010d,
	kHF14KeyCode				= 0x010e,
	kHF15KeyCode				= 0x010f
};

#ifndef PATH_MAX
#define NAME_MAX	256
#define PATH_MAX	1024
#endif

#ifndef NAME_MAX
#define NAME_MAX	256
#endif

#endif
