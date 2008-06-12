#include "MRS.h"

#include <vector>
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
	string key(inIndex);
	
	DataMap::iterator m = mIndexedData.find(key);
	if (m == mIndexedData.end())
		mIndexedData.insert(key, new CIndexData(kTextIndex, inText));
	else if (m->second->index_kind == kTextIndex)
		m->second->text.push_back(inText);
	else
		THROW(("Inconsistent use of indices for index %s", inIndex));
}

void CDocument::AddIndexNumber(
	const char*		inIndex,
	const char*		inNumber)
{
	string key(inIndex);
	
	DataMap::iterator m = mIndexedData.find(key);
	if (m == mIndexedData.end())
		mIndexedData.insert(key, new CIndexData(kNumberIndex, inNumber));
	else if (m->second->index_kind == kNumberIndex)
		m->second->text.push_back(inNumber);
	else
		THROW(("Inconsistent use of indices for index %s", inIndex));
}

void CDocument::AddIndexDate(
	const char*		inIndex,
	const char*		inDate)
{
	string key(inIndex);
	
	DataMap::iterator m = mIndexedData.find(key);
	if (m == mIndexedData.end())
		mIndexedData.insert(key, new CIndexData(kDateIndex, inDate));
	else if (m->second->index_kind == kDateIndex)
		m->second->text.push_back(inDate);
	else
		THROW(("Inconsistent use of indices for index %s", inIndex));
}

void CDocument::AddIndexValue(
	const char*		inIndex,
	const char*		inValue)
{
	string key(inIndex);
	
	DataMap::iterator m = mIndexedData.find(key);
	if (m == mIndexedData.end())
		mIndexedData.insert(key, new CIndexData(kValueIndex, inValue));
	else
		THROW(("Already set value for index %s", inIndex));
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
	
	for (DataMap::iterator dm = mIndexedData.begin(); dm != mIndexedData.end(); ++dm)
	{
		auto_ptr<CIndexTokens> it(new CIndexTokens);

		it->index_kind = dm->second->index_kind;
		it->index_name = dm->first;

		for (vector<string>::iterator text = dm->second->text.begin(); text != dm->second->text.end(); ++text)
		{
			if (it->index_kind == kTextIndex)
			{
				CTokenizer tok(text->c_str(), text->length());
				bool isWord, isNumber, isPunct;
				
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
					
					string word(tok.GetTokenValue(), l);
					ba::to_lower(word);
			
					if (l <= kMaxKeySize)
						it->tokens.push_back(inLexicon.Store(word));
					else	
						it->tokens.push_back(0);
				}
			}
			else
			{
				ba::to_lower(*text);
				it->tokens.push_back(inLexicon.Store(*text));
			}
		}

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
			map<string,string>::iterator mf = mMetaData.find(*m);
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
