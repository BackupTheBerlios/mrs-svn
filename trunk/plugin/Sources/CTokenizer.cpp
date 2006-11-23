/*	$Id: CTokenizer.cpp 69 2006-05-27 20:15:32Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 19:10:48
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
 
#include "MRS.h"

#include <iostream>

#include "CTokenizer.h"

using namespace std;

// On some OS's isalnum is implemented as a system-call... duh
// I believe we can do better

namespace fast
{

const uint8
	kCharIsAlphaMask = 0x01,
	kCharIsDigitMask = 0x02,
	kCharPropTable[128] = {
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 
		0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 
		0x0, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 
		0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 
	};

inline bool isalnum(char c)
{
	return c >= 0x30 and kCharPropTable[static_cast<uint32>(c)] != 0;
}

inline bool isalpha(char c)
{
	return c >= 0x40 and (kCharPropTable[static_cast<uint32>(c)] & kCharIsAlphaMask) != 0;
}

inline bool isdigit(char c)
{
	return c >= 0x30 and (kCharPropTable[static_cast<uint32>(c)] & kCharIsDigitMask) != 0;
}

inline char tolower(char c)
{
	const char kToLowerMask = 0x20;
	
	if (isalpha(c))
		c |= kToLowerMask;

	return c;
}

}

CTokenizer::CTokenizer(const char* inData, uint32 inLength)
	: eof(false)
	, buffer(inData)
	, buffer_size(inLength)
	, p(buffer)
{
	assert(inLength > 0);
}

char CTokenizer::GetNextChar()
{
	char result = 0;
	
	if (not unget.empty())
	{
		result = unget.top();
		unget.pop();
	}
	else if (not eof)
	{
		assert(p >= buffer);
		assert(p < buffer + buffer_size);
		
		result = fast::tolower(*p++);
	
		if (p >= buffer + buffer_size)
			eof = true;
			
			// avoid returning a zero if not end of file detected
		if (not eof and result == 0)
			result = ' ';
	}
	
	*t++ = result;
	
	return result;
}

inline
void CTokenizer::Retract()
{
	assert(t > tokenText);
	--t;
	unget.push(*t);
	*t = 0;
}

inline
int CTokenizer::Restart(int inStart)
{
	while (t > tokenText)
		Retract();
	
	int result = 0;
	switch (inStart)
	{
		case 10:	result = 20; break;
		case 20:	result = 30; break;
	}
	return result;
}

bool CTokenizer::GetToken(bool& outWord, bool& outNumber)
{
	bool result = true;
	int32 dots = 0;
	int32 hyphens = 0;

	t = tokenText;
	
	int state = 10;
	int start = 10;
	
	enum {
		tok_None,
		tok_Number,
		tok_Word,
		tok_Other
	} token = tok_None;
	
	while (token == tok_None and t < tokenText + kMaxTokenLength)
	{
		char c = GetNextChar();
		
		switch (state)
		{
			case 10:
				if (c == 0)	// done!
				{
					result = false;
					token = tok_Other;
				}
				else if (fast::isdigit(c))		// first try a number
					state = 11;
				else if (fast::isalnum(c) or c == '_')
					state = 21;
				else
					state = start = 30;
				break;
			
			case 11:
				if (c == '.')
					state = 12;
				else if (fast::isalpha(c) or c == '_')
					state = start = Restart(start);
				else if (not fast::isdigit(c))	// integer
				{
					Retract();
					token = tok_Number;
				}
				break;
			
			case 12:						// check for a trailing digit
				if (fast::isalnum(c) or c == '_')
				{
					state = 13;
				}
				else
				{
					Retract();
					Retract();
					token = tok_Number;
				}
				break;
			
			case 13:				// some identifier that started with a digit
				if (c == '.')		// watch out, we only accept one period in between alnum's
				{
					state = 14;
				}
				else if (not (fast::isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;
			
			case 14:
				if (fast::isalnum(c) or c == '_')
				{
					++dots;
					state = 13;
				}
				else
				{
					Retract();
					Retract();
					token = tok_Word;
				}
				break;
			
			// parse identifiers
			case 20:
				if (fast::isalnum(c) or c == '_')
					state = 21;
				else
					state = start = Restart(start);
				break;
			
			case 21:
				if (c == '.')
					state = 22;
				else if (c == '-')
					state = 25;
				else if (not (fast::isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;

			case 22:
				if (fast::isalnum(c) or c == '_')
						// watch out, we only accept one period in between alnum's
					state = 23;
				else
				{
					Retract();
					Retract();
					token = tok_Word;
				}
				break;
			
			case 23:
				if (c == '.')
					state = 22;
				else if (not (fast::isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;
			
			case 24:
				if (fast::isalnum(c) or c == '_')
					state = 13;
				else
				{
					Retract();
					Retract();
					token = tok_Word;
				}
				break;
			
			// no trailing dashes and no double dashes allowed
			case 25:
				if (c == '-' or not fast::isalnum(c))
				{
					Retract();
					Retract();
					token = tok_Word;
				}
				else
				{
					++hyphens;
					state = 21;
				}
				break;
			
			case 30:
				if (fast::isalnum(c) or c == '_' or c == 0)
				{
					Retract();
					token = tok_Other;
				}
				break;
		}
	}
	
	// can happen if token is too long:
	if (token == tok_None and t > tokenText)
	{
		token = tok_Other;
	}

	switch (token)
	{
		case tok_Other:
			outNumber = false;
			outWord = false;
			break;
		
		case tok_Number:
			outNumber = true;
			outWord = false;
			break;
		
		case tok_Word:
		default:
			outNumber = false;
			outWord = true;
			
	// words with just one hypen and no dots are split
			if (hyphens == 1 and dots == 0)
			{
				char* d;
				for (d = t - 1; d > tokenText and *d != '-'; --d)
					unget.push(*d);
				
				assert(*d == '-');
				t = d;
			}
			break;
	}

	*t = 0;

	return result;
}

void CTokenizer::SetOffset(uint32 inOffset)
{
	if (inOffset > buffer_size)
		THROW(("Parameter for CTokenizer::SetOffset is out of range"));

	p = buffer + inOffset;
}
