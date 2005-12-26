/*	$Id: CTokenizer.h,v 1.22 2005/09/15 10:12:08 maarten Exp $
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
 
#ifndef CTOKENIZER_H
#define CTOKENIZER_H

#include "HStdCCtype.h"
#include "HStlStack.h"
#include "HStream.h"
#include "HError.h"

template<int M>
class CTokenizer
{
  public:

	typedef char	EntryText[M + 1];

					CTokenizer(HStreamBase* inSource, int64 inOffset, int64 inLength);
					CTokenizer(const char* inData, uint32 inLength);
	virtual			~CTokenizer();

	bool			GetToken(EntryText& outToken, bool& outWord, bool& outNumber);
	void			Reset();
	
	int64			GetOffset()		{ return data_offset + pos - buffer_size + (p - buffer) - unget.size(); }
	void			SetOffset(int64 inOffset);

  private:
	
	static const uint32	kBufferSize = 4096;

	char			GetNextChar();
	int				Restart(int inStart);
	void			Retract();

	HStreamBase*	data;
	int64			data_offset;
	int64			data_size;
	int64			pos;
	bool			eof;
	char*			buffer;
	uint32			buffer_size;
	char*			p;
	char*			t;
	char*			ts;
	std::stack<char>unget;
};

template<int M>
CTokenizer<M>::CTokenizer(HStreamBase* inData, int64 inOffset, int64 inLength)
	: data(inData)
	, data_offset(inOffset)
	, data_size(inLength)
{
	buffer = new char[kBufferSize];
	Reset();
}

template<int M>
CTokenizer<M>::CTokenizer(const char* inData, uint32 inLength)
	: data(NULL)
	, data_offset(0)
	, data_size(inLength)
	, pos(inLength)
	, eof(false)
	, buffer(const_cast<char*>(inData))
	, buffer_size(inLength)
	, p(buffer)
{
	assert(inLength > 0);
}

template<int M>
CTokenizer<M>::~CTokenizer()
{
	if (data)
		delete[] buffer;
}

template<int M>
void CTokenizer<M>::Reset()
{
	assert(data != nil);
	
	eof = false;
	
	data->Seek(data_offset, SEEK_SET);
	
	uint32 k = static_cast<uint32>(kBufferSize);
	if (k > data_size)
		k = static_cast<uint32>(data_size);
	data->Read(buffer, k);
	buffer_size = k;
	pos = kBufferSize;
	p = buffer;
}

template<int M>
char CTokenizer<M>::GetNextChar()
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
		
		result = *p++;
	
		if (p >= buffer + buffer_size)
		{
			if (pos < data_size and data != nil)
			{
				uint32 k = static_cast<uint32>(data_size - pos);
				if (k > kBufferSize)
					k = kBufferSize;
			
				data->Read(buffer, k);
				buffer_size = k;
				pos += k;
			}
			else
				eof = true;
			
			p = buffer;
		}

			// avoid returning a zero if not end of file detected		
		if (not eof and result == 0)
			result = ' ';
	}
	
	*t++ = result;
	
	return result;
}

template<int M>
inline
void CTokenizer<M>::Retract()
{
	assert(t > ts);
	--t;
	unget.push(*t);
	*t = 0;
}

template<int M>
inline
int CTokenizer<M>::Restart(int inStart)
{
	while (t > ts)
		Retract();
	
	int result = 0;
	switch (inStart)
	{
		case 10:	result = 20; break;
		case 20:	result = 30; break;
	}
	return result;
}

template<int M>
bool CTokenizer<M>::GetToken(EntryText& outToken, bool& outWord, bool& outNumber)
{
	using namespace std;

	bool result = true;
	bool hasDot = false;
	bool hasHyphen = false;

	t = ts = outToken;
	
	int state = 10;
	int start = 10;
	
	enum {
		tok_None,
		tok_Number,
		tok_Word,
		tok_Other
	} token = tok_None;
	
	while (token == tok_None and t < ts + M)
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
				else if (isdigit(c))		// first try a number
					state = 11;
				else
					state = start = Restart(start);
				break;
			
			case 11:
				if (c == '.')
					state = 12;
				else if (isalpha(c) or c == '_')
					state = start = Restart(start);
				else if (not isdigit(c))	// integer
				{
					Retract();
					token = tok_Number;
				}
				break;
			
			case 12:						// trailing digit
				if (isalnum(c) or c == '_')
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
				else if (not (isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;
			
			case 14:
				if (isalnum(c) or c == '_')
				{
					hasDot = true;
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
				if (isalnum(c) or c == '_')
					state = 21;
				else
					state = start = Restart(start);
				break;
			
			case 21:
				if (c == '.')
					state = 22;
				else if (c == '-')
					state = 25;
				else if (not (isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;

			case 22:
				if (isalnum(c) or c == '_')
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
				else if (not (isalnum(c) or c == '_'))
				{
					Retract();
					token = tok_Word;
				}
				break;
			
			case 24:
				if (isalnum(c) or c == '_')
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
				if (c == '-' or not isalnum(c))
				{
					Retract();
					Retract();
					token = tok_Word;
				}
				else
				{
					hasHyphen = true;
					state = 21;
				}
				break;
			
			case 30:
				if (isalnum(c) or c == '_')
				{
					Retract();
					token = tok_Other;
				}
				break;
		}
	}
	
	// can happen if token is too long:
	if (token == tok_None and t > ts)
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
			
			if (hasDot or hasHyphen)
			{
				for (; ts < t and *ts != '.' and *ts != '-'; ++ts)
					;
				
				for (char* d = t - 1; d > ts; --d)
					unget.push(*d);
			}
			break;
	}

	*t = 0;
	
	return result;
}

template<int M>
void CTokenizer<M>::SetOffset(int64 inOffset)
{
	if (inOffset < 0 or inOffset >= data_size)
		THROW(("Invalid offset passed to CTokenizer::SetOffset"));
	
	// only works with char buffers, not with streams... implement if needed
	assert(data == nil);
	
	p = buffer + inOffset;
	
	while (not unget.empty())
		unget.pop();
}

#endif // CTOKENIZER_H
