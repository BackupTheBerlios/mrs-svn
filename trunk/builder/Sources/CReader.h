#ifndef CREADER_H
#define CREADER_H

#include <boost/shared_ptr.hpp>
#include <boost/filesystem/path.hpp>

class CReader;
typedef boost::shared_ptr<CReader>	CReaderPtr;

class CReader
{
  public:
	
						~CReader();

	static CReader*		CreateReader(
							boost::filesystem::path&
												inFile);

	static CReaderPtr	sEnd;

	bool				GetLine(
							std::string&		outLine);

	bool				Eof();

	static void			GetStatistics(
							int64&				outReadRawData);

  private:
						CReader(
							struct CReaderImp*	inImpl);
	
						CReader(
							const CReader&);
	CReader&			operator=(
							const CReader&);

	struct CReaderImp*	mImpl;
};

#endif
