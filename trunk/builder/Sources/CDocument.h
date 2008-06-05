#ifndef CDOCUMENT_H
#define CDOCUMENT_H

#include <boost/shared_ptr.hpp>
#include <map>
#include <vector>

class CDocument;
typedef boost::shared_ptr<CDocument> CDocumentPtr;

class CDocument
{
  public:
	typedef std::map<std::string,std::string>	DataMap;

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

  private:
	std::string			mID;
	std::string			mText;
	DataMap				mMetaData;
	DataMap				mIndexedTextData;
	DataMap				mIndexedValueData;
	std::vector<std::string>
						mSequences;
};

#endif
