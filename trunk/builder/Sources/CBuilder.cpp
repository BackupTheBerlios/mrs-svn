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
#include <iomanip>

#include "HFile.h"
#include "HBuffer.h"

#include "HBuffer.h"
#include "CDatabank.h"
#include "CParser.h"
#include "CReader.h"
#include "CLexicon.h"
#include "CDocument.h"
#include "CCompress.h"

using namespace std;
namespace fs = boost::filesystem;

int VERBOSE = 0;
int COMPRESSION_LEVEL = 9;
unsigned int THREADS = 2;
const char* COMPRESSION_DICTIONARY = "";
const char* COMPRESSION = "zlib";

// ------------------------------------------------------------------

fs::path		gRawDir("/data/raw/");
fs::path		gMrsDir("/data/mrs/");

// ------------------------------------------------------------------

typedef boost::shared_ptr<CReader>		CReaderPtr;
typedef HBuffer<CReaderPtr,100>			CReaderBuffer;

typedef boost::shared_ptr<CDocument>	CDocumentPtr;
typedef HBuffer<CDocumentPtr,100>		CDocumentBuffer;

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

	void		ParseFiles(
					const string&		inScriptName,
					CReaderBuffer*		inReaderBuffer,
					CDocumentBuffer*	inCompressDocBuffer);
	
	void		ProcessDocument(
					CDocumentPtr		inDocument,
					void*				inUserData);

	void		CompressDoc(
					CDocumentBuffer*	inCompressDocBuffer,
					CDocumentBuffer*	inTokenizeDocBuffer);

	void		TokenizeDoc(
					CDocumentBuffer*	inTokenizeDocBuffer,
					CDocumentBuffer*	inIndexDocBuffer);
	
	void		IndexDoc(
					CDocumentBuffer*	inIndexDocBuffer);
	
	CDatabank*	mDatabank;
	string		mDatabankName, mScriptName;
	CParser*	mParser;
	CCompressorFactory*
				mCompressorFactory;
	vector<string>
				mMeta;
	HFile::SafeSaver*
				mSafe;
	CLexicon	mLexicon;
};

CBuilder::CBuilder(
	const string&			inDatabank,
	const string&			inScript)
	: mDatabank(nil)
	, mDatabankName(inDatabank)
	, mScriptName(inScript)
	, mParser(nil)
	, mCompressorFactory(nil)
	, mSafe(nil)
{
	try
	{
		mParser = new CParser(mScriptName);
		
		string name, version, url, section;
		
		mParser->GetInfo(name, version, url, section, mMeta);
		
		fs::path file = gMrsDir / (inDatabank + ".cmp");
		HUrl path;
		path.SetNativePath(file.string());
		
		if (fs::exists(file))
		{
			mSafe = new HFile::SafeSaver(path);
			path = mSafe->GetURL();
		}
		
#warning("fix me")
		mCompressorFactory = new CCompressorFactory("zlib", 3, "", 0);
		
		mDatabank = new CDatabank(path, mMeta, inDatabank,
			version, url, inScript, section, *mCompressorFactory);
	}
	catch (...)
	{
		if (mParser != nil)
			delete mParser;
		
		if (mDatabank != nil)
			delete mDatabank;
		
		throw;
	}
}

CBuilder::~CBuilder()
{
	delete mParser;
	delete mDatabank;
	delete mSafe;
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
	
	boost::thread_group parser_threads, compress_threads;
	uint32 nrOfParsers = 2;
	if (nrOfParsers > rawFiles.size())
		nrOfParsers = rawFiles.size();

	CReaderBuffer readerBuffer;
	CDocumentBuffer compressDocBuffer, tokenizeDocBuffer, indexDocBuffer;
	
	for (uint32 i = 0; i < nrOfParsers; ++i)
	{
		parser_threads.create_thread(boost::bind(&CBuilder::ParseFiles, this, mScriptName, &readerBuffer, &compressDocBuffer));
		compress_threads.create_thread(boost::bind(&CBuilder::CompressDoc, this, &compressDocBuffer, &tokenizeDocBuffer));
	}
	
	boost::thread tokenizeDocThread(boost::bind(&CBuilder::TokenizeDoc, this, &tokenizeDocBuffer, &indexDocBuffer));
	boost::thread indexDocThread(boost::bind(&CBuilder::IndexDoc, this, &indexDocBuffer));
	
	for (vector<fs::path>::iterator file = rawFiles.begin(); file != rawFiles.end(); ++file)
	{
		if (VERBOSE > 1)
			cerr << '\r'
				 << setw(32) << file->leaf()
				 << " ["
				 << setw(6) << (file - rawFiles.begin()) + 1
				 << '/'
				 << setw(6) << rawFiles.size()
				 << ']';
		
		CReaderPtr reader(CReader::CreateReader(*file));
		readerBuffer.Put(reader);
	}
	
	readerBuffer.Put(CReader::sEnd);
	
	parser_threads.join_all();
	compress_threads.join_all();
	tokenizeDocThread.join();
	indexDocThread.join();

	if (VERBOSE > 1)
		cout << endl << "done" << endl;
	
	mDatabank->Finish(mLexicon, true, false);
	delete mDatabank;
	mDatabank = nil;
	
	if (mSafe != nil)
		mSafe->Commit();
}

void CBuilder::CollectRawFiles(
	vector<fs::path>&	outRawFiles,
	int64&				outRawFilesSize)
{
	fs::path rawDir = gRawDir / mDatabankName;
	
	outRawFilesSize = 0;
	
	if (not fs::exists(rawDir))
		THROW(("Raw directory %s does not exist", rawDir.string().c_str()));
	
	fs::directory_iterator end_itr;
	for (fs::directory_iterator itr(rawDir); itr != end_itr; ++itr)
	{
		if (mParser->IsRawFile(itr->leaf()))
		{
			outRawFiles.push_back(*itr);
			outRawFilesSize += fs::file_size(*itr);
		}
	}
}

void CBuilder::ParseFiles(
	const string&		inScriptName,
	CReaderBuffer*		inReaderBuffer,
	CDocumentBuffer*	inCompressDocBuffer)
{
	CParser parser(inScriptName);

	CReaderPtr next;
	while ((next = inReaderBuffer->Get()) != CReader::sEnd)
		parser.Parse(*next, inCompressDocBuffer,
			boost::bind(&CBuilder::ProcessDocument, this, _1, _2));
	
	inReaderBuffer->Put(CReader::sEnd);
	inCompressDocBuffer->Put(CDocument::sEnd);
}

void CBuilder::ProcessDocument(
	CDocumentPtr		inDocument,
	void*				inUserData)
{
	CDocumentBuffer* compressDocBuffer = reinterpret_cast<CDocumentBuffer*>(inUserData);
	compressDocBuffer->Put(inDocument);
}

void CBuilder::CompressDoc(
	CDocumentBuffer*	inCompressDocBuffer,
	CDocumentBuffer*	inTokenizeDocBuffer)
{
	CDocumentPtr next;
	auto_ptr<CCompressor> compressor(mCompressorFactory->CreateCompressor());
	
	while ((next = inCompressDocBuffer->Get()) != CDocument::sEnd)
	{
		next->Compress(mMeta, *compressor);
		inTokenizeDocBuffer->Put(next);
	}
	
	inCompressDocBuffer->Put(CDocument::sEnd);
	inTokenizeDocBuffer->Put(CDocument::sEnd);
}

void CBuilder::TokenizeDoc(
	CDocumentBuffer*	inTokenizeDocBuffer,
	CDocumentBuffer*	inIndexDocBuffer)
{
	CDocumentPtr next;
	
	while ((next = inTokenizeDocBuffer->Get()) != CDocument::sEnd)
	{
		next->TokenizeText(mLexicon);
		inIndexDocBuffer->Put(next);
	}
	
	inIndexDocBuffer->Put(CDocument::sEnd);
}

void CBuilder::IndexDoc(
	CDocumentBuffer*	inIndexDocBuffer)
{
	CDocumentPtr next;
	
	while ((next = inIndexDocBuffer->Get()) != CDocument::sEnd)
	{
		mDatabank->StoreDocument(*next);

		const CDocument::TokenMap& tokenData = next->GetTokenData();

		for (CDocument::TokenMap::const_iterator d = tokenData.begin(); d != tokenData.end(); ++d)
		{
			if (d->is_value)
				mDatabank->IndexValue(d->index, d->tokens[0]);
			else
				mDatabank->IndexTokens(d->index, d->tokens);
		}

		mDatabank->FlushDocument();
	}
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
		CBuilder builder(databank, script);
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
