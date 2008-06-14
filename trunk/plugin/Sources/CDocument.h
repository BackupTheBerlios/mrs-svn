#ifndef CDOCUMENT_H
#define CDOCUMENT_H

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <map>
#include <vector>
#include <limits>
#include "HStream.h"
#include "CLexicon.h"

class CDocument;
typedef boost::shared_ptr<CDocument> CDocumentPtr;

class CCompressor;

class CDocument
{
  public:

	enum {
		kUndefinedTokenValue = ~0UL
	};
	
	struct CTokenData
	{
		uint32					doc_token;
		uint32					global_token;
	};

	struct CIndexTokens
	{
		CIndexKind				index_kind;
		std::string				index_name;
		std::vector<CTokenData>	tokens;
	};

	typedef boost::ptr_vector<CIndexTokens>	CTokenMap;

						CDocument();

						~CDocument();

	void				SetMetaData(
							const char*		inField,
							const char*		inText);

	void				SetText(
							const char*		inText);
	
	void				AddIndexText(
							const char*		inIndex,
							const char*		inText,
							bool			inIndexNumbers);

	void				AddIndexWord(
							const char*		inIndex,
							const char*		inWord);

	void				AddIndexNumber(
							const char*		inIndex,
							const char*		inNumber);
	
	void				AddIndexDate(
							const char*		inIndex,
							const char*		inDate);
	
	void				AddIndexValue(
							const char*		inIndex,
							const char*		inValue);

	void				AddSequence(
							const char*		inSequence);
	
	static CDocumentPtr	sEnd;

	void				TokenizeText(
							CLexicon&		inLexicon,
							uint32			inLastStopWord);

	void				Compress(
							const std::vector<std::string>&
											inMetaDataFields,
							CCompressor&	inCompressor);

	const void*			Data() const			{ return mCompressedData.Buffer(); }

	int64				Size() const			{ return mCompressedData.Size(); }

	uint32				TextLength() const		{ return mText.length(); }

	const CTokenMap&	GetTokenMap() const		{ return mTokenData; }

  private:

	CTokenMap::iterator	GetTokenData(
							const std::string&
											inKey,
							CIndexKind		inKind);

	uint32				StoreToken(
							const char*		inToken,
							uint32			inLength);

	std::string			mText;
	std::map<std::string,std::string>
						mMetaData;
	CTokenMap			mTokenData;
	std::vector<std::string>
						mSequences;
	HMemoryStream		mCompressedData;
	
	CLexicon			mDocLexicon;
};

#endif
