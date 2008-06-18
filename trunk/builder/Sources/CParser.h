#ifndef CPARSER_H
#define CPARSER_H

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/filesystem/path.hpp>
#include <vector>

class CDocument;
class CReader;
typedef boost::shared_ptr<CDocument>	CDocumentPtr;

namespace fs = boost::filesystem;

class CParser
{
  public:

	typedef boost::function<void(CDocumentPtr, void*)>	CDocCallback;

						CParser(
							const std::string&		inDatabank,
							const std::string&		inScriptName,
							const std::string&		inRawDir);

						~CParser();
	
	void				CollectRawFiles(
							std::vector<fs::path>&	outRawFiles);

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

	void				GetCompressionInfo(
							std::string&			outCompressionAlgorithm,
							int32&					outCompressionLevel,
							std::string&			outCompressionDictionary);

  private:

						CParser(
							const CParser&);
	CParser&			operator=(
							const CParser&);

	struct CParserImp*	mImpl;
};

#endif
