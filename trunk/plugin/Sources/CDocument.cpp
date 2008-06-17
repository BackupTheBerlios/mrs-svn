#include "MRS.h"

#include <vector>
#include <iostream>
#include <boost/algorithm/string.hpp>

#include "CLexicon.h"
#include "CTokenizer.h"
#include "CDocument.h"
#include "CCompress.h"

using namespace std;
namespace ba = boost::algorithm;

extern const uint32 kMaxKeySize;

CDocumentPtr CDocument::sEnd;

CDocument::CDocument()
	 : mDocLexicon(false)
{
}

CDocument::~CDocument()
{
}

void CDocument::SetText(
	const char*		inText)
{
	assert(mText.length() == 0);
	mText.assign(inText);
}

void CDocument::SetMetaData(
	const char*		inField,
	const char*		inText)
{
	mMetaData[inField] = inText;
}

CDocument::CTokenMap::iterator CDocument::GetTokenData(
	const string&	inIndex,
	CIndexKind		inKind)
{
	CTokenMap::iterator result = mTokenData.begin();
	while (result != mTokenData.end() and result->index_name != inIndex)
		++result;
	
	if (result == mTokenData.end())
	{
		mTokenData.push_back(new CIndexTokens);
		result = mTokenData.end() - 1;
		result->index_name = inIndex;
		result->index_kind = inKind;
	}
	else if (result->index_kind != inKind)
		THROW(("Inconsistent use of indices for index %s", inIndex.c_str()));
	
	return result;
}

void CDocument::AddIndexText(
	const char*		inIndex,
	const char*		inText,
	bool			inIndexNrs)
{
	CTokenMap::iterator ix = GetTokenData(inIndex, kTextIndex);
	
	CTokenizer tok(inText, strlen(inText));
	bool isWord, isNumber, isPunct;
	
	while (tok.GetToken(isWord, isNumber, isPunct))
	{
		CTokenData t = { 0, kUndefinedTokenValue };
		
		const char* w = tok.GetTokenValue();
		uint32 l = tok.GetTokenLength();
		
		if (not (isWord or isNumber) or l == 0)
			continue;
		
		if (isPunct or (isNumber and not inIndexNrs) or l > kMaxKeySize)
		{
			ix->tokens.push_back(t);
			continue;
		}
		
		char word[kMaxKeySize];
		char* word_end = ba::to_lower_copy(word,
			boost::iterator_range<const char*>(w, w + l));
		
		t.doc_token = mDocLexicon.Store(word, word_end - word);

		ix->tokens.push_back(t);
	}
}

void CDocument::AddIndexWord(
	const char*		inIndex,
	const char*		inWord)
{
	CTokenMap::iterator ix = GetTokenData(inIndex, kTextIndex);
	
	string value(inWord);
	ba::to_lower(value);
	
	CTokenData t;
	t.doc_token = mDocLexicon.Store(value);
	t.global_token = kUndefinedTokenValue;
	ix->tokens.push_back(t);
}

void CDocument::AddIndexNumber(
	const char*		inIndex,
	const char*		inNumber)
{
	CTokenMap::iterator ix = GetTokenData(inIndex, kNumberIndex);
	
	string value(inNumber);
	ba::to_lower(value);
	
	CTokenData t;
	t.doc_token = mDocLexicon.Store(value);
	t.global_token = kUndefinedTokenValue;
	ix->tokens.push_back(t);
}

void CDocument::AddIndexDate(
	const char*		inIndex,
	const char*		inDate)
{
	CTokenMap::iterator ix = GetTokenData(inIndex, kDateIndex);
	
	string value(inDate);
	ba::to_lower(value);
	
	CTokenData t;
	t.doc_token = mDocLexicon.Store(value);
	t.global_token = kUndefinedTokenValue;
	ix->tokens.push_back(t);
}

void CDocument::AddIndexValue(
	const char*		inIndex,
	const char*		inValue)
{
	CTokenMap::iterator ix = GetTokenData(inIndex, kValueIndex);
	
	string value(inValue);
	ba::to_lower(value);
	
	CTokenData t;
	t.doc_token = mDocLexicon.Store(value);
	t.global_token = kUndefinedTokenValue;
	ix->tokens.push_back(t);
}

void CDocument::AddSequence(
	const char*		inSequence)
{
	mSequences.push_back(inSequence);
}

void CDocument::TokenizeText(
	CLexicon&		inLexicon,
	uint32			inLastStopWord)
{
	uint32 docTokenCount = mDocLexicon.Count();
	vector<uint32> tokenRemap(docTokenCount, kUndefinedTokenValue);
	tokenRemap[0] = 0;
	
	try
	{
		inLexicon.LockShared();
		
		for (uint32 t = 1; t < docTokenCount; ++t)
		{
			const char* w;
			uint32 wl;
			
			mDocLexicon.GetString(t, w, wl);
			uint32 rt = inLexicon.Lookup(w, wl);

			if (rt != 0)
				tokenRemap[t] = rt;
		}
		
		inLexicon.UnlockShared();
	}
	catch (...)
	{
		inLexicon.UnlockShared();
		throw;
	}
	
	try
	{
		inLexicon.LockUnique();
		
		for (uint32 t = 1; t < docTokenCount; ++t)
		{
			if (tokenRemap[t] == kUndefinedTokenValue)
			{
				const char* w;
				uint32 wl;
				
				mDocLexicon.GetString(t, w, wl);
				tokenRemap[t] = inLexicon.Store(w, wl);
			}
		}
		
		inLexicon.UnlockUnique();
	}
	catch (...)
	{
		inLexicon.UnlockUnique();
		throw;
	}
	
	for (CTokenMap::iterator tm = mTokenData.begin(); tm != mTokenData.end(); ++tm)
	{
		for (vector<CTokenData>::iterator td = tm->tokens.begin(); td != tm->tokens.end(); ++td)
		{
			if (tokenRemap[td->doc_token] > inLastStopWord)
				td->global_token = tokenRemap[td->doc_token];
			else
				td->global_token = 0;
			
			assert(td->global_token != kUndefinedTokenValue);
		}
	}
}

void CDocument::Compress(
	const vector<string>&	inMetaDataFields,
	CCompressor&			inCompressor)
{
	if (inMetaDataFields.size() > 0)
	{
		vector<pair<const char*,uint32> > dv;
		
		for (vector<string>::const_iterator m = inMetaDataFields.begin(); m != inMetaDataFields.end(); ++m)
		{
			map<string,string>::iterator mf = mMetaData.find(*m);
			if (mf != mMetaData.end())
				dv.push_back(make_pair(mf->second.c_str(), mf->second.length()));
			else
				dv.push_back(make_pair("", 0));
		}

		dv.push_back(make_pair(mText.c_str(), mText.length()));

		inCompressor.CompressData(dv, mCompressedData);
	}
	else
		inCompressor.CompressDocument(mText.c_str(), mText.length(), mCompressedData);
}

void CDocument::Write(
	HStreamBase&	inFile) const
{
	inFile.Write(mCompressedData.Buffer(), mCompressedData.Size());
}
