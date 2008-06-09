#include "MRS.h"

#include <vector>

#include "CLexicon.h"
#include "CTokenizer.h"
#include "CDocument.h"
#include "CCompress.h"

using namespace std;

extern const uint32 kMaxKeySize;

CDocumentPtr CDocument::sEnd;

CDocument::CDocument()
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

void CDocument::AddIndexText(
	const char*		inIndex,
	const char*		inText)
{
	mIndexedTextData[inIndex] += inText;
	mIndexedTextData[inIndex] += "\n";
}

void CDocument::AddIndexValue(
	const char*		inIndex,
	const char*		inValue)
{
	mIndexedValueData[inIndex] = inValue;
}

void CDocument::AddSequence(
	const char*		inSequence)
{
	mSequences.push_back(inSequence);
}

void CDocument::TokenizeText(
	CLexicon&		inLexicon)
{
	bool inIndexNrs = true;
	
	for (DataMap::iterator dm = mIndexedTextData.begin(); dm != mIndexedTextData.end(); ++dm)
	{
		CTokenizer tok(dm->second.c_str(), dm->second.length());
		bool isWord, isNumber, isPunct;
		
		auto_ptr<CIndexTokens> it(new CIndexTokens);

		it->is_value = false;
		it->index = dm->first;
	
		while (tok.GetToken(isWord, isNumber, isPunct))
		{
			uint32 l = tok.GetTokenLength();
			
			if (isPunct or (isNumber and not inIndexNrs))
			{
				it->tokens.push_back(0);
				continue;
			}
			
			if (not (isWord or isNumber) or l == 0)
				continue;
	
			if (l <= kMaxKeySize)
			{
				uint32 word = inLexicon.Store(string(tok.GetTokenValue(), l));
				it->tokens.push_back(word);
			}
			else	
				it->tokens.push_back(0);
		}
		
		mTokenData.push_back(it.release());
	}

	for (DataMap::iterator dm = mIndexedValueData.begin(); dm != mIndexedValueData.end(); ++dm)
	{
		auto_ptr<CIndexTokens> it(new CIndexTokens);

		it->is_value = true;
		it->index = dm->first;
		it->tokens.push_back(inLexicon.Store(dm->second));
		
		mTokenData.push_back(it.release());
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
			DataMap::iterator mf = mMetaData.find(*m);
			if (mf != mMetaData.end())
				dv.push_back(make_pair(mf->second.c_str(), mf->second.length()));
			else
				dv.push_back(make_pair("", 0));
		}

		dv.push_back(make_pair(mText.c_str(), mText.length()));

		inCompressor.CompressData(dv, mData);
	}
	else
		inCompressor.CompressDocument(mText.c_str(), mText.length(), mData);
}
