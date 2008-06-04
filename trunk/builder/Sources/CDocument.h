#ifndef CDOCUMENT_H
#define CDOCUMENT_H

#include <boost/shared_ptr.hpp>

class CDocument;
typedef boost::shared_ptr<CDocument> CDocumentPtr;

class CDocument
{
  public:
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

	
	
	static CDocumentPtr	sEnd;
};

#endif
