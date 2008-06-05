#ifndef CDOCUMENT_H
#define CDOCUMENT_H

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <map>
#include <vector>

class CDocument;
typedef boost::shared_ptr<CDocument> CDocumentPtr;

class CLexicon;

class CDocument
{
  public:
	typedef std::map<std::string,std::string>	DataMap;

	struct CIndexTokens
	{
		bool					is_value;
		std::string				index;
		std::vector<uint32>		tokens;
	};

	typedef boost::ptr_vector<CIndexTokens>		TokenMap;

						CDocument();
						~CDocument();

	void				SetText(
							const char*		inText);
	
	void				SetMetaData(
							const char*		inField,
							const char*		inText);
	
	void				AddIndexText(
							const char*		inIndex,
							const char*		inText);
	
	void				AddIndexValue(
							const char*		inIndex,
							const char*		inValue);

	void				AddSequence(
							const char*		inSequence);
	
	static CDocumentPtr	sEnd;

	const std::string&	GetText()			{ return mText; }
	const std::string&	GetID()				{ return mID; }
	
	const DataMap&		GetMetaData()		{ return mMetaData; }
	const DataMap&		GetIndexedTextData()
											{ return mIndexedTextData; }
	const DataMap&		GetIndexedValueData()
											{ return mIndexedValueData; }
	const TokenMap&		GetTokenData()
											{ return mTokenData; }

	void				TokenizeText(
							CLexicon&		inLexicon);

  private:
	std::string			mID;
	std::string			mText;
	DataMap				mMetaData;
	DataMap				mIndexedTextData;
	DataMap				mIndexedValueData;
	TokenMap			mTokenData;
	std::vector<std::string>
						mSequences;
};

#endif
