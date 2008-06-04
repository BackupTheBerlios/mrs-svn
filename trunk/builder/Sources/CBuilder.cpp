/*
	A new build driver application for MRS.
	
	Idea is to use as much multithreading as makes sense.
*/

#include "MRS.h"

#include <boost/filesystem.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>
#include <getopt.h>
#include <string>
#include <iostream>

#include "HBuffer.h"

#include "CDatabank.h"
#include "CParser.h"
#include "CReader.h"

using namespace std;
namespace fs = boost::filesystem;

int VERBOSE = 0;

// ------------------------------------------------------------------

class CBuilder
{
  public:
				CBuilder(
					const string&		inDatabank,
					const string&		inScript);

				~CBuilder();
	
	void		Run();

  private:

	void		CollectRawFiles(
					vector<fs::path>&	outRawFiles,
					int64&				outRawFilesSize);

	void		IndexDoc(
					CDocumentBuffer*	inIndexDocBuffer,
					CDocumentBuffer*	inCompressDocBuffer);
	
	void		CompressDoc(
					CDocumentBuffer*	inCompressDocBuffer);

	CDatabank*	mDatabank;
	string		mDatabankName, mScriptName;
};

CBuilder::CBuilder(
	const string&			inDatabank,
	const string&			inScript)
	: mDatabank(nil)
	, mDatabankName(inDatabank)
	, mScriptName(inScript)
{
}

CBuilder::~CBuilder()
{
}

void CBuilder::Run()
{
	vector<fs::path> rawFiles;
	int64 totalRawFilesSize;
	
	if (VERBOSE > 1)
		cerr << "Collecting raw file names...";
	
	CollectRawFiles(rawFiles, totalRawFilesSize);
	
	if (VERBOSE > 1)
		cerr << "done" << endl;
	
	if (VERBOSE > 0)
		cerr << "Processing " << rawFiles.size() << " raw files totalling " << totalRawFilesSize << " bytes" << endl;
	
	boost::ptr_vector<CParser> parsers;
	boost::thread_group parser_threads;
	uint32 nrOfParsers = 4;
	if (nrOfParsers > rawFiles.size())
		nrOfParsers = rawFiles.size();

	CReaderBuffer readerBuffer;
	CDocumentBuffer indexDocBuffer, compressDocBuffer;
	
	for (uint32 i = 0; i < nrOfParsers; ++i)
	{
		CParser* parser = new CParser(mScriptName);
		parsers.push_back(parser);
		parser_threads.create_thread(boost::bind(&CParser::Run, parser, &readerBuffer, &indexDocBuffer));
	}
	
	boost::thread indexDocThread(boost::bind(&CBuilder::IndexDoc, this, &indexDocBuffer, &compressDocBuffer));
	boost::thread compressDocThread(boost::bind(&CBuilder::CompressDoc, this, &compressDocBuffer));
	
	for (vector<fs::path>::iterator file = rawFiles.begin(); file != rawFiles.end(); ++file)
	{
		if (VERBOSE > 1)
			cerr << "Reading file " << file->string() << endl;
		
		CReaderPtr reader(CReader::CreateReader(*file));
		readerBuffer.Put(reader);
	}
	
	readerBuffer.Put(CReader::sEnd);
	
	parser_threads.join_all();
	indexDocThread.join();
	compressDocThread.join();
	
	mDatabank->Finish(true, false);
}

void CBuilder::CollectRawFiles(
	vector<fs::path>&	outRawFiles,
	int64&				outRawFilesSize)
{
}

void CBuilder::IndexDoc(
	CDocumentBuffer*	inIndexDocBuffer,
	CDocumentBuffer*	inCompressDocBuffer)
{
}

void CBuilder::CompressDoc(
	CDocumentBuffer*	inCompressDocBuffer)
{
}

// ------------------------------------------------------------------

void usage()
{
	cout << "usage ..." << endl;
	exit(1);
}

void error(const char* msg, ...)
{
	cout << msg << endl;
	exit(1);
}

int main(int argc, char* const argv[])
{
	string script, databank;
	
	int c;
	
	while ((c = getopt(argc, argv, "s:d:v")) != -1)
	{
		switch (c)
		{
			case 's':
				script = optarg;
				break;
			
			case 'd':
				databank = optarg;
				break;
			
			case 'v':
				++VERBOSE;
				break;
			
			default:
				usage();
		}
	}

	if (databank.length() == 0)
		usage();
	
	if (script.length() == 0)
		script = databank;

	try
	{
		CBuilder builder(script, databank);
		builder.Run();
	}
	catch (exception& e)
	{
		cerr << "mrsbuild failed with exception:" << endl << e.what() << endl;
		exit(1);
	}
	catch (...)
	{
		cerr << "mrsbuild failed with unhandled exception" << endl;
		exit(1);
	}
	
	return 0;
}
