#ifndef CREADER_H
#define CREADER_H

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

class CReader;
typedef boost::shared_ptr<CReader>	CReaderPtr;

class CReader
{
  public:
	
	static CReader*		CreateReader(
							boost::filesystem::path&
												inFile);

	static CReaderPtr	sEnd;

	bool				GetLine(
							std::string&		outLine);

	bool				Eof();

  private:
						CReader();
	
	struct CReaderImp*	mImpl;
};

#endif
