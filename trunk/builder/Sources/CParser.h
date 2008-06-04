#ifndef CPARSER_H
#define CPARSER_H

#include "HBuffer.h"
#include <boost/shared_ptr.hpp>

class CReader;
typedef boost::shared_ptr<CReader>		CReaderPtr;
typedef HBuffer<CReaderPtr>				CReaderBuffer;

class CDocument;
typedef boost::shared_ptr<CDocument>	CDocumentPtr;
typedef HBuffer<CDocumentPtr>			CDocumentBuffer;

class CParser
{
  public:
						CParser(
							const std::string&		inScriptName);

						~CParser();
	
	void				Run(
							CReaderBuffer*			inReaders,
							CDocumentBuffer*		inDocBuffer);

  private:
	struct CParserImp*	mImpl;
};

#endif
