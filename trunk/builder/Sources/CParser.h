#ifndef CPARSER_H
#define CPARSER_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <vector>

class CDocument;
class CReader;
typedef boost::shared_ptr<CDocument>	CDocumentPtr;

class CParser
{
  public:

	typedef boost::function<void(CDocumentPtr, void*)>	CDocCallback;

						CParser(
							const std::string&		inScriptName);

						~CParser();
	
	bool				IsRawFile(
							const std::string&		inFile);

	void				Parse(
							CReader&				inReader,
							void*					inUserData,
							CDocCallback			inCallback);

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
