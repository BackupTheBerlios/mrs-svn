/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Monday December 02 2002 15:31:25
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
#include "MRSInterface.h"

#include "HStlList.h"
#include "HStlIOStream.h"
#include "HStlSet.h"
#include "HFile.h"
#include "HError.h"
#include "HUtils.h"
#include "HStream.h"
#include "HStlLimits.h"

#include "zlib.h"
#include "bzlib.h"

#include "CCompress.h"
#include "CDatabank.h"
#include "CBitStream.h"
#include "CTokenizer.h"
#include "CTextTable.h"
#include "CArray.h"

using namespace std;

const uint32
	kFirstPassCutOff = 5000000,
	kMinWordOccurence = 10,
	kMaxDictionarySize = 0x7fff,
	kCompressionLevel = 9,
	kMaxWordsLexiconSize = 250000,
	kMaxNonWordsLexiconSize = 25000,
	kMaxNumberLength = 4;
	

struct CCompressorImp
{
					CCompressorImp(HStreamBase& inData, uint32 inSig);
	virtual			~CCompressorImp();
	
	virtual void	Init(HStreamBase& /*inText*/) {}
	virtual void	CompressDocument(const char* inText, uint32 inSize) = 0;
	
	static CCompressorImp* Create(HStreamBase& inData, const string& inCompressionType);
	
	HSwapStream<net_swapper>	data;
	uint32						sig;
};

CCompressorImp::CCompressorImp(HStreamBase& inData, uint32 inSig)
	: data(inData)
	, sig(inSig)
{
}

CCompressorImp::~CCompressorImp()
{
}

namespace {

// ZLib compression

class CZLibDictionary
{
  public:
				CZLibDictionary();
	
	void		AddToken(const char* inText);
	uint32		Size() const;
	string		Finish();
	
  private:

	struct CompareStrings
	{
		bool operator() (const char* a, const char* b) const
		{
			return strcmp(a, b) < 0;
		}
	};
	
	struct CntCompare
	{
						CntCompare(uint32* inArray)
							: arr(inArray) {}
		
		bool			operator() (uint32 a, uint32 b) const
							{ return arr[a] > arr[b]; } 
		
		uint32*	arr;
	};

	typedef map<const char*, uint32, CompareStrings>	Index;

	CTextTable	text;
	Index		index;
};

CZLibDictionary::CZLibDictionary()
{
}

void CZLibDictionary::AddToken(const char* inText)
{
	Index::iterator i = index.lower_bound(inText);

	bool done = false;

	if (i != index.end())
	{
		uint32 na = strlen(inText);
		uint32 nb = strlen((*i).first);
		
		if (na <= nb && strncmp(inText, (*i).first, na) == 0)
		{
			++(*i).second;
			done = true;
		}
	}

	if (not done && i != index.begin())
	{
		Index::iterator j = i;
		--j;
		
		uint32 na = strlen(inText);
		uint32 nb = strlen((*j).first);

		if (na > nb && strncmp((*j).first, inText, nb) == 0)
		{
			uint32 cnt = (*j).second + 1;
			index.erase(j);
			index[text.Store(inText)] = cnt;

			done = true;
		}
	}		
	
	if (not done)
		index[text.Store(inText)] = 1;
}

uint32 CZLibDictionary::Size() const
{
	return index.size();
}

string CZLibDictionary::Finish()
{
	// now we've got a bunch of words, create a string
	// with the most used words at the end
	
	uint32 n, h;
	uint32* A;

	// try to reduce the lexicon size to something reasonable
	Index::iterator w;

	n = index.size();
	A = new uint32[n * 2];
	
	const char** str = new const char*[n];
	
	uint32 s = 0;
	uint32 i = 0;
	for (w = index.begin(); w != index.end(); ++w, ++i)
	{
		A[i] = i + n;
		A[i + n] = (*w).second;
		str[i] = (*w).first;
		s += strlen(str[i]);
	}
	
	h = n;
	make_heap(A, A + h, CntCompare(A));
	
	// reduce lexiconsize by removing the least common words
	// and words that occur less than kMinWordOccurence
	
	while (h > 1 && (s > kMaxDictionarySize || A[A[0]] < kMinWordOccurence))
	{
		const char* t = str[A[0] - n];
		
		s -= strlen(t);
		A[0] = A[h - 1];
		--h;
		pop_heap(A, A + h, CntCompare(A));
	}
	
	string dictionary;
	
	while (h > 0)
	{
		dictionary.append(str[A[0] - n]);

		A[0] = A[h - 1];
		--h;
		if (h > 0)
			pop_heap(A, A + h, CntCompare(A));
	}
	
	delete[] str;
	delete[] A;
	
	return dictionary;
}

}

struct CZLibCompressorImp : public CCompressorImp
{
					CZLibCompressorImp(HStreamBase& inData);
					~CZLibCompressorImp();
	
	virtual void	Init(HStreamBase& inText);
	virtual void	CompressDocument(const char* inText, uint32 inSize);
	
	z_stream_s		z_stream;
	string			dictionary;
};

CZLibCompressorImp::CZLibCompressorImp(HStreamBase& inData)
	: CCompressorImp(inData, kZLibCompressed)
{
	memset(&z_stream, 0, sizeof(z_stream));

	int compressionLevel = kCompressionLevel;
	if (COMPRESSION_LEVEL >= 0 and COMPRESSION_LEVEL <= 9)
		compressionLevel = COMPRESSION_LEVEL;

	int err = deflateInit(&z_stream, compressionLevel);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));
}

CZLibCompressorImp::~CZLibCompressorImp()
{
	deflateEnd(&z_stream);
}

void CZLibCompressorImp::Init(HStreamBase& inText)
{
	CZLibDictionary lexicon;

	if (inText.Size() > 0)
	{
		bool isWord, isNumber;
		CTokenizer<15>::EntryText text;
		CTokenizer<15> tok(&inText, 0, inText.Tell());
		
		while (tok.GetToken(text, isWord, isNumber))
		{
			if (strlen(text) < 4)
				continue;

			lexicon.AddToken(text);
		}
	}

	dictionary = lexicon.Finish();
	
	dictionary += COMPRESSION_DICTIONARY;
	if (dictionary.length() > kMaxDictionarySize)
		dictionary.erase(0, dictionary.length() - kMaxDictionarySize);
	
	uint16 ds = static_cast<uint16>(dictionary.length());
	data << ds;
	data.Write(dictionary.c_str(), dictionary.length());
}

void CZLibCompressorImp::CompressDocument(const char* inText, uint32 inSize)
{
	const int kBufferSize = 4096;
	static HAutoBuf<unsigned char> sBuffer(new unsigned char[kBufferSize]);

	z_stream.next_in = const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(inText));
	z_stream.avail_in = inSize;
	z_stream.total_in = 0;
	
	z_stream.next_out = sBuffer.get();
	z_stream.avail_out = kBufferSize;
	z_stream.total_out = 0;
	
	int err = deflateReset(&z_stream);
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	err = deflateSetDictionary(&z_stream,
		reinterpret_cast<const unsigned char*>(dictionary.c_str()), dictionary.length());
	if (err != Z_OK)
		THROW(("Compressor error: %s", z_stream.msg));

	for (;;)
	{
		err = deflate(&z_stream, Z_FINISH);
		
		if (z_stream.total_out > 0)
			data.Write(sBuffer.get(), z_stream.total_out);
		
		if (err == Z_OK)
		{
			z_stream.next_out = sBuffer.get();
			z_stream.avail_out = kBufferSize;
			z_stream.total_out = 0;
			continue;
		}
		
		break;
	}
	
	if (err != Z_STREAM_END)
		THROW(("Deflate error: %s (%d)", z_stream.msg, err));
}

struct CbzLibCompressorImp : public CCompressorImp
{
					CbzLibCompressorImp(HStreamBase& inData);
					~CbzLibCompressorImp();
	
//	virtual void	Init(HStreamBase& inText);
	virtual void	CompressDocument(const char* inText, uint32 inSize);
	
	bz_stream		fStream;
	int				fCompressionLevel;
};

CbzLibCompressorImp::CbzLibCompressorImp(HStreamBase& inData)
	: CCompressorImp(inData, kbzLibCompressed)
{
	memset(&fStream, 0, sizeof(fStream));

	fCompressionLevel = kCompressionLevel;
	if (COMPRESSION_LEVEL >= 0 and COMPRESSION_LEVEL <= 9)
		fCompressionLevel = COMPRESSION_LEVEL;
}

CbzLibCompressorImp::~CbzLibCompressorImp()
{
}

void CbzLibCompressorImp::CompressDocument(const char* inText, uint32 inSize)
{
	int err = BZ2_bzCompressInit(&fStream, fCompressionLevel, 0, 0);
	if (err != BZ_OK)
		THROW(("Compressor error: %d", err));

	const int kBufferSize = 4096;
	static HAutoBuf<char> sBuffer(new char[kBufferSize]);

	fStream.next_in = const_cast<char*>(inText);
	fStream.avail_in = inSize;
	fStream.total_in_lo32 = 0;
	fStream.total_in_hi32 = 0;
	
	fStream.next_out = sBuffer.get();
	fStream.avail_out = kBufferSize;
	fStream.total_out_lo32 = 0;
	fStream.total_out_hi32 = 0;
	
	int action = BZ_RUN;
	
	for (;;)
	{
		err = BZ2_bzCompress(&fStream, action);
		action = BZ_FINISH;
		
		if (fStream.total_out_lo32 > 0)
			data.Write(sBuffer.get(), fStream.total_out_lo32);
		
		if (err == BZ_FINISH_OK or err == BZ_RUN_OK)
		{
			fStream.next_out = sBuffer.get();
			fStream.avail_out = kBufferSize;
			fStream.total_out_lo32 = 0;
			continue;
		}
		
		break;
	}
	
	if (err != BZ_STREAM_END)
		THROW(("bzCompress error: %d", err));

	BZ2_bzCompressEnd(&fStream);
	memset(&fStream, 0, sizeof(fStream));
}

// huffword compression

struct CompareStrings
{
	bool operator() (const char* a, const char* b) const
	{
		return strcmp(a, b) < 0;
	}
};

class CCmpLexicon
{
  public:
						CCmpLexicon(uint32 inMaxSize);
						~CCmpLexicon();

	void				Reduce(HStreamBase& inFile);
	
	void				AddToken(const char* inText);
	void				WriteToken(const char* inText, COBitStream& ioBits);
	
  private:

	struct LexEntry
	{
		const char*		text;
		uint32			cnt;
		uint32			code;
	
						LexEntry()
							: text(NULL), cnt(0), code(0) {}
						LexEntry(const char* inText, uint32 inCount)
							: text(inText), cnt(inCount), code(0) {}
						LexEntry(const LexEntry& inOther)
							: text(inOther.text), cnt(inOther.cnt), code(inOther.code) {}
		
		bool			operator<(const LexEntry& inOther) const
							{ return strcmp(text, inOther.text) < 0; }
	};

	typedef vector<LexEntry> 								Lexicon;
	typedef std::map<const char*, uint32, CompareStrings>	LexiconSet;

	struct RestChar
	{
		unsigned char	ch;
		uint32			cnt;
		uint32			code;
		
		bool			operator< (const RestChar& inOther) const
							{ return ch < inOther.ch; }
	};
	
	typedef vector<RestChar>	RestCharacters;
		
	struct CntCompare
	{
						CntCompare(uint32* inArray)
							: arr(inArray) {}
		
		bool			operator() (uint32 a, uint32 b) const
							{ return arr[a] > arr[b]; } 
		
		uint32*	arr;
	};
	
	uint32					max_size;
	CTextTable				text;
	char*					symbol_text;
	Lexicon					lexicon;
	RestCharacters			rest;
	LexiconSet				word_set;
};

CCmpLexicon::CCmpLexicon(uint32 inMaxSize)
	: max_size(inMaxSize)
	, symbol_text(NULL)
{
}

CCmpLexicon::~CCmpLexicon()
{
	delete symbol_text;
}

void CCmpLexicon::AddToken(const char* inText)
{
	LexiconSet::iterator i = word_set.find(inText);
	if (i == word_set.end())
		word_set[text.Store(inText)] = 1;
	else
		++(*i).second;
}

inline
void Push(COBitStream& inBits, uint32 inCode, uint32 inLen)
{
	for (int i = static_cast<int>(inLen) - 1; i >= 0; --i)
		inBits << ((inCode & (1 << i)) != 0);
}

void CCmpLexicon::WriteToken(const char* inText, COBitStream& ioBits)
{
	Lexicon::iterator i = lower_bound(lexicon.begin() + 1, lexicon.end(), LexEntry(inText, 0));
	if (i != lexicon.end() && strcmp(inText, (*i).text) == 0)
	{
		Push(ioBits, (*i).code, (*i).cnt);
	}
	else
	{
		Push(ioBits, lexicon[0].code, lexicon[0].cnt);

		for (const char* p = inText; *p; ++p)
		{
			RestChar e;
			e.ch = static_cast<unsigned char>(*p);
			RestCharacters::iterator i = lower_bound(rest.begin(), rest.end(), e); 
			assert(i != rest.end());
			assert((*i).ch == static_cast<unsigned char>(*p));
			
			Push(ioBits, (*i).code, (*i).cnt);
		}
			
		Push(ioBits, rest[0].code, rest[0].cnt);
	}
}

void CCmpLexicon::Reduce(HStreamBase& inFile)
{
	HSwapStream<net_swapper> data(inFile);
	
	lexicon.push_back(LexEntry("\x1b", 0));
	
	RestCharacters chars;
	uint32 i;
	
	for (i = 0; i < 256; ++i)
	{
		chars.push_back(RestChar());
		chars[i].ch = static_cast<unsigned char>(i);
		chars[i].cnt = 1;	// was 0
		chars[i].code = 0;
	}

	uint32 n, h;

	// try to reduce the lexicon size to something reasonable
	LexiconSet::iterator w;

	n = word_set.size();
	HAutoBuf<uint32> A_(new uint32[n * 2]);
	uint32* A = A_.get();
	
	HAutoBuf<const char*> str(new const char*[n]);
	
	uint32 s = 0;
	i = 0;
	for (w = word_set.begin(); w != word_set.end(); ++w, ++i)
	{
		A[i] = i + n;
		A[i + n] = (*w).second;
		str[i] = (*w).first;
		s += strlen(str[i]) + 1;
	}

//	word_set.clear();
	word_set = LexiconSet();
	
	h = n;
	make_heap(A, A + h, CntCompare(A));
	
	while (s > max_size)
	{
		const char* t = str[A[0] - n];
		
		++lexicon.front().cnt;
		
		for (const char* p = t; *p; ++p)
			++chars[static_cast<unsigned char>(*p)].cnt;
		++chars[0].cnt;
		
		s -= strlen(t) + 1;
		A[0] = A[h - 1];
		--h;
		pop_heap(A, A + h, CntCompare(A));
	}
	
	for (i = 0; i < h; ++i)
		lexicon.push_back(LexEntry(str[A[i] - n], A[A[i]]));

	sort(lexicon.begin() + 1, lexicon.end());
	
	n = lexicon.size();
	A = new uint32[n * 2];

	for (i = 0; i < n; ++i)
	{
		A[i] = i + n;
		A[i + n] = lexicon[i].cnt;
	}
	
	h = n;
	make_heap(A, A + h, CntCompare(A));
	
	while (h > 1)
	{
		uint32 m1 = A[0];
		A[0] = A[h - 1];
		--h;
		pop_heap(A, A + h, CntCompare(A));
		
		uint32 m2 = A[0];
		A[0] = A[h - 1];
		
		A[h] = A[m1] + A[m2];
		A[0] = h;
		A[m1] = A[m2] = h;
		
		pop_heap(A, A + h);
	}
	
	A[1] = 0;
	for (i = 2; i < 2 * n; ++i)
		A[i] = A[A[i]] + 1;
	
	for (i = 0; i < n; ++i)
		lexicon[i].cnt = A[i + n];

	uint32 numl[32];
	uint32 firstcode[32];
	uint32 nextcode[32];
	
	for (i = 0; i < 32; ++i)
		numl[i] = 0;
	
	for (i = 0; i < n; ++i)
		++numl[A[i + n]];
	
	firstcode[31] = 0;
	for (int l = 30; l >= 0; --l)
		firstcode[l] = (firstcode[l + 1] + numl[l + 1]) / 2;
	
	for (int l = 0; l < 32; ++l)
		nextcode[l] = firstcode[l];
	
	HAutoBuf<uint32> symbol_table(new uint32[n]);
	
	uint32 six[32];
	six[0] = 0;
	for (i = 1; i < 32; ++i)
		six[i] = six[i - 1] + numl[i - 1];
	
	for (i = 0; i < n; ++i)
	{
		uint32 li = A[i + n];
		
		lexicon[i].code = nextcode[li];
		symbol_table[six[li] + nextcode[li] - firstcode[li]] = i;
		++nextcode[li];
	}
	
	data << n;
	for (i = 0; i < 32; ++i)	data << firstcode[i];
	for (i = 0; i < 32; ++i)	data << six[i];

	uint32 symbol_text_length = 0;
	for (i = 0; i < n; ++i)
		symbol_text_length += strlen(lexicon[symbol_table[i]].text) + 1;
	
	symbol_text = new char[symbol_text_length];
	char* d = symbol_text;
	
	for (i = 0; i < n; ++i)
	{
		strcpy(d, lexicon[symbol_table[i]].text);
		lexicon[symbol_table[i]].text = d;
		symbol_table[i] = static_cast<uint32>(d - symbol_text);
		d += strlen(d) + 1;
	}

	data << symbol_text_length;
	data.Write(symbol_text, symbol_text_length);

	// and now repeat all steps for the rest characters
	
	// Count how many characters we actually have:
	n = 0;
	
	rest = chars;
//	for (RestCharacters::iterator i = chars.begin(); i != chars.end(); ++i)
//	{
//		if ((*i).cnt != 0)
//		{
//			rest.push_back(*i);
//			rest.back().cnt = 0;
//		}
//	}

	n = rest.size();
	A_.reset(new uint32[n * 2]);
	A = A_.get();

	for (i = 0; i < n; ++i)
	{
		A[i] = i + n;
		A[i + n] = rest[i].cnt;
	}
	
	h = n;
	make_heap(A, A + h, CntCompare(A));
	
	while (h > 1)
	{
		uint32 m1 = A[0];
		A[0] = A[h - 1];
		--h;
		pop_heap(A, A + h, CntCompare(A));
		
		uint32 m2 = A[0];
		A[0] = A[h - 1];
		
		A[h] = A[m1] + A[m2];
		A[0] = h;
		A[m1] = A[m2] = h;
		
		pop_heap(A, A + h);
	}
	
	A[1] = 0;
	for (i = 2; i < 2 * n; ++i)
		A[i] = A[A[i]] + 1;
	
	for (i = 0; i < n; ++i)
		rest[i].cnt = A[i + n];

	for (i = 0; i < 32; ++i)
		numl[i] = 0;
	
	for (i = 0; i < n; ++i)
		++numl[A[i + n]];
	
	firstcode[31] = 0;
	for (int l = 30; l >= 0; --l)
		firstcode[l] = (firstcode[l + 1] + numl[l + 1]) / 2;
	
	for (int l = 0; l < 32; ++l)
		nextcode[l] = firstcode[l];
	
	six[0] = 0;
	for (i = 1; i < 32; ++i)
		six[i] = six[i - 1] + numl[i - 1];
	
	HAutoBuf<unsigned char> char_symbol_table(new unsigned char[n]);
	
	for (i = 0; i < n; ++i)
	{
		uint32 li = A[i + n];
		
		rest[i].code = nextcode[li];
		char_symbol_table[six[li] + nextcode[li] - firstcode[li]] = rest[i].ch;
		++nextcode[li];
	}
	
	data << n;
	for (i = 0; i < 32; ++i)	data << firstcode[i];
	for (i = 0; i < 32; ++i)	data << six[i];
	data.Write(char_symbol_table.get(), n);
}

struct CHuffWordCompressorImp : public CCompressorImp
{
					CHuffWordCompressorImp(HStreamBase& inData);
					~CHuffWordCompressorImp();
	
	virtual void	Init(HStreamBase& inText);
	virtual void	CompressDocument(const char* inText, uint32 inSize);

	CCmpLexicon*	words;
	CCmpLexicon*	non_words;
};

CHuffWordCompressorImp::CHuffWordCompressorImp(HStreamBase& inData)
	: CCompressorImp(inData, kHuffWordCompressed)
	, words(new CCmpLexicon(kMaxWordsLexiconSize))
	, non_words(new CCmpLexicon(kMaxNonWordsLexiconSize))
{
}

CHuffWordCompressorImp::~CHuffWordCompressorImp()
{
	delete words;
	delete non_words;
}

void CHuffWordCompressorImp::Init(HStreamBase& inText)
{
	bool word = true, isWord, isNumber;
	CTokenizer<15> tok(&inText, 0, inText.Tell());
	CTokenizer<15>::EntryText text;
	
	while (tok.GetToken(text, isWord, isNumber))
	{
		if (isWord)
		{
			if (not word)
				non_words->AddToken("");
			
			if (not isNumber)
				words->AddToken(text);
			else
			{
				char n[kMaxNumberLength + 1];
				char* t = text;

				for (;;)
				{
					uint32 i;
					for (i = 0; i < kMaxNumberLength && t[i] != 0; ++i)
						n[i] = t[i];

					n[i] = 0;
					t += i;
					
					words->AddToken(n);
					
					if (*t == 0)
						break;

					non_words->AddToken("");
				}
			}
			
			word = false;
		}
		else
		{
			if (word)
				words->AddToken("");
			non_words->AddToken(text);
			word = true;
		}
	}
	
	words->Reduce(data);
	non_words->Reduce(data);
}

void CHuffWordCompressorImp::CompressDocument(const char* inText, uint32 inSize)
{
	bool word = true, isWord, isNumber;
	CTokenizer<15> tok(inText, inSize);
	CTokenizer<15>::EntryText text;
	COBitStream bits;
	
	while (tok.GetToken(text, isWord, isNumber))
	{
		if (isWord)
		{
			if (not word)
				non_words->WriteToken("", bits);
			
			if (not isNumber)
				words->WriteToken(text, bits);
			else
			{
				char n[kMaxNumberLength + 1];
				char* t = text;

				for (;;)
				{
					uint32 i;
					for (i = 0; i < kMaxNumberLength && t[i] != 0; ++i)
						n[i] = t[i];

					n[i] = 0;
					t += i;
					
					words->WriteToken(n, bits);
					
					if (*t == 0)
						break;

					non_words->WriteToken("", bits);
				}
			}
			
			word = false;
		}
		else
		{
			if (word)
				words->WriteToken("", bits);
			non_words->WriteToken(text, bits);
			word = true;
		}
	}
	
	bits.sync();
	data.Write(bits.peek(), bits.size());
}

CCompressorImp* CCompressorImp::Create(HStreamBase& inData, const string& inCompressionType)
{
	CCompressorImp* result = NULL;
	
	if (inCompressionType == "huffword")
	{
		result = new CHuffWordCompressorImp(inData);
		if (VERBOSE)
			cout << "Using huffword compression" << endl;
	}
	else if (inCompressionType == "zlib")
	{
		result = new CZLibCompressorImp(inData);
		if (VERBOSE)
			cout << "Using zlib compression" << endl;
	}
	else if (inCompressionType == "bzip")
	{
		result = new CbzLibCompressorImp(inData);
		if (VERBOSE)
			cout << "Using bzip compression" << endl;
	}
	else
		THROW(("Unknown compression type"));

	return result;
}

CCompressor::CCompressor(HStreamBase& inData, const HUrl& inDb)
	: fImpl(nil)
	, fData(inData)
	, fScratch(nil)
	, fDataOffset(0)
	, fFirstDocOffset(0)
	, fDocIndexData(nil)
	, fDocCount(0)
{
	fOffsetsUrl = HUrl(inDb.GetURL() + ".offsets");
	fDocIndexData = new HTempFileStream(fOffsetsUrl);

	fScratchUrl = HUrl(inDb.GetURL() + ".scratch");
	fScratch = new HTempFileStream(fScratchUrl);

	fImpl = CCompressorImp::Create(inData, COMPRESSION);
}

CCompressor::~CCompressor()
{
	delete fImpl;
	delete fScratch;
	delete fDocIndexData;
}

void CCompressor::AddDocument(const char* inText, uint32 inSize)
{
	assert(inSize > 0);

	if (fFirstDocOffset == 0)
		fFirstDocOffset = fData.Tell();
	
	if (fScratch)
	{
		fScratch->Write(inText, inSize);
		fScratchIndex.push_back(fScratch->Tell());
		
		if (fScratchIndex.back() > kFirstPassCutOff)
			ProcessScratch();
	}
	else
		CompressDocument(inText, inSize);

	++fDocCount;
}

void CCompressor::Finish(int64& outDataOffset, int64& outDataSize,
	int64& outTableOffset, int64& outTableSize,
	uint32& outCompressionKind, uint32& outCount)
{
	if (fScratch)
		ProcessScratch();

	outCompressionKind = fImpl->sig;
	outCount = fDocCount;

	outDataOffset = fDataOffset;
	outDataSize = fData.Tell() - fDataOffset;

	outTableOffset = fData.Tell();
	CCArray<int64> arr(*fDocIndexData, outDataSize);
	fData.Write(arr.Peek(), arr.Size());
	outTableSize = fData.Tell() - outTableOffset;
}

void CCompressor::ProcessScratch()
{
	fDataOffset = fData.Tell();
	
	fImpl->Init(*fScratch);

	fFirstDocOffset = fData.Tell();
	(*fDocIndexData) << 0ULL;
	
	int64 last = 0;
	for (vector<int64>::iterator i = fScratchIndex.begin(); i != fScratchIndex.end(); ++i)
	{
		uint32 size = static_cast<uint32>(*i - last);
		HAutoBuf<char> text(new char[size]);
		fScratch->PRead(text.get(), size, last);
		last = *i;
		
		CompressDocument(text.get(), size);
	}
	
	delete fScratch;
	fScratch = NULL;
	
	fScratchIndex = std::vector<int64>();

	HFile::Unlink(fScratchUrl);
}

void CCompressor::CompressDocument(const char* inText, uint32 inSize)
{
	fImpl->CompressDocument(inText, inSize);
	(*fDocIndexData) << (fData.Tell() - fFirstDocOffset);
}

