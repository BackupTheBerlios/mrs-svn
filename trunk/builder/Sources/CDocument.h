#ifndef CDOCUMENT_H
#define CDOCUMENT_H

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <map>
#include <vector>
#include "HStream.h"

class CDocument;
typedef boost::shared_ptr<CDocument> CDocumentPtr;

class CLexicon;
class CCompressor;

class CDocument
{
  public:
	struct CIndexData
	{
		uint32					 index_kind;
		std::vector<std::string> text;

								CIndexData(
									uint32		inKind,
									const char*	inText)
									: index_kind(inKind)
								{
									text.push_back(inText);
								}
	};
	
	typedef boost::ptr_map<std::string,CIndexData>	DataMap;		

	struct CIndexTokens
	{
		uint32					index_kind;
		std::string				index_name;
		std::vector<uint32>		tokens;
	};

	typedef boost::ptr_vector<CIndexTokens>			TokenMap;

						CDocument();

						~CDocument();

	void				SetMetaData(
							const char*		inField,
							const char*		inText);

	void				SetText(
							const char*		inText);
	
	void				AddIndexText(
							const char*		inIndex,
							const char*		inText);

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

	const DataMap&		GetIndexedData()		{ return mIndexedData; }

	const TokenMap&		GetTokenData()			{ return mTokenData; }

	void				TokenizeText(
							CLexicon&		inLexicon);

	void				Compress(
							const std::vector<std::string>&
											inMetaDataFields,
							CCompressor&	inCompressor);

	const void*			Data() const			{ return mData.Buffer(); }
	int64				Size() const			{ return mData.Size(); }
	uint32				TextLength() const		{ return mText.length(); }

  private:
	std::string			mText;
	std::map<std::string,std::string>
						mMetaData;
	DataMap				mIndexedData;
	TokenMap			mTokenData;
	std::vector<std::string>
						mSequences;
	HMemoryStream		mData;
};

#endif
