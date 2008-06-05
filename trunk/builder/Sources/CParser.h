#ifndef CPARSER_H
#define CPARSER_H

#include "HBuffer.h"
#include <boost/shared_ptr.hpp>
#include <vector>

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

	bool				IsRawFile(
							const std::string&		inFile);

	void				GetInfo(
							std::string&			outName,
							std::string&			outVersion,
							std::string&			outUrl,
							std::string&			outSection,
							std::vector<std::string>&
													outMetaDataFields);

  private:

						CParser(
							const CParser&);
	CParser&			operator=(
							const CParser&);

	struct CParserImp*	mImpl;
};

#endif
