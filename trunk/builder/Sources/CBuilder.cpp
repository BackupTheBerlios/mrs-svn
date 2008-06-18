/*
	A new build driver application for MRS.
	
	Idea is to use as much multithreading as makes sense.
*/

#include "MRS.h"

#include <boost/iterator/iterator_facade.hpp>
#include <boost/thread.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <getopt.h>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdarg>

#include "HFile.h"
#include "HBuffer.h"
#include "HUtils.h"

#include "CDatabank.h"
#include "CParser.h"
#include "CReader.h"
#include "CLexicon.h"
#include "CDocument.h"
#include "CCompress.h"

using namespace std;
namespace fs = boost::filesystem;
namespace ba = boost::algorithm;

// ------------------------------------------------------------------

fs::path		gRawDir("/data/raw/");
fs::path		gMrsDir("/data/mrs/");

// ------------------------------------------------------------------

typedef boost::shared_ptr<CReader>		CReaderPtr;
typedef HBuffer<CReaderPtr,10>			CReaderBuffer;

typedef boost::shared_ptr<CDocument>	CDocumentPtr;
typedef HBuffer<CDocumentPtr,10>		CDocumentBuffer;

class CBuilder : public CDBBuildProgressMixin
{
  public:
				CBuilder(
					const string&		inDatabank,
					const string&		inScript);

				~CBuilder();
	
	void		Run(
					int32				inNrOfPipeLines,
					bool				inShowProgress);

	void		ReadStopWords();

	void		Progress();

  private:

	void		CollectRawFiles(
					vector<fs::path>&	outRawFiles);

	void		ParseFiles(
					const string&		inScriptName,
					CReaderBuffer*		inReaderBuffer,
					CDocumentBuffer*	inCompressDocBuffer);
	
	void		ProcessDocument(
					CDocumentPtr		inDocument,
					void*				inUserData);
	
	void		TokenizeDoc(
					CDocumentBuffer*	inTokenizeDocBuffer,
					CDocumentBuffer*	inCompressDocBuffer);

	void		CompressDoc(
					CDocumentBuffer*	inCompressDocBuffer,
					CDocumentBuffer*	inIndexDocBuffer);
	
	void		IndexDoc(
					CDocumentBuffer*	inIndexDocBuffer,
					int32				inNrOfParsers);

	virtual void
				SetCreateIndexProgress(
					int64		inCurrentLexEntry,
					int64		inTotalLexEntries);
		
	virtual void
				SetWritingIndexProgress(
					uint32		inCurrentKey,
					uint32		inKeyCount,
					const char*	inIndexName);

	
	CDatabank*	mDatabank;
	string		mDatabankName, mScriptName;
	fs::path	mRawDir;
	CParser*	mParser;
	CCompressorFactory*
				mCompressorFactory;
	vector<string>
				mMeta;
	HFile::SafeSaver*
				mSafe;
	CLexicon	mLexicon;
	uint32		mLastStopWord;
	
	// progress information
	bool		mParsingDone, mShowProgress;
	double		mStartTime, mLastUpdate;
	uint32		mFileCount;
	uint32		mCurrentFile;
	int64		mRawDataSize;
	string		mLastRawFile;
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
	, mLexicon(true)
	, mLastStopWord(0)
	, mFileCount(0)
	, mCurrentFile(0)
	, mRawDataSize(0)
{
	try
	{
		mRawDir = gRawDir / mDatabankName;
		
		mParser = new CParser(mDatabankName, mScriptName, mRawDir.string());
		
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
		
		string compAlgo, compDict;
		int32 compLevel;
		
		mParser->GetCompressionInfo(compAlgo, compLevel, compDict);
		
		mCompressorFactory = new CCompressorFactory(compAlgo, compLevel, compDict);
		
		mDatabank = new CDatabank(path, mMeta, inDatabank,
			version, url, inScript, section, this, *mCompressorFactory, mLexicon);
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

void CBuilder::Run(
	int32		inNrOfPipeLines,				
	bool		inShowProgress)
{
	mShowProgress = inShowProgress;

	vector<fs::path> rawFiles;
	
	if (VERBOSE > 1)
		cerr << "Collecting raw file names...";
	
	CollectRawFiles(rawFiles);
	
	if (VERBOSE > 1)
		cerr << "done" << endl;
	
	if (rawFiles.size() == 0)
		THROW(("No files to process, aborting"));
	
	mFileCount = rawFiles.size();
	
	if (VERBOSE > 0)
		cerr << "Processing " << rawFiles.size() << " raw files totalling " << mRawDataSize << " bytes" << endl;

	// we now start parsing
	mParsingDone = false;
	mStartTime = system_time();

	boost::thread_group parser_threads, tokenize_threads, compress_threads;
	uint32 nrOfParsers = inNrOfPipeLines;
	if (nrOfParsers > rawFiles.size())
		nrOfParsers = rawFiles.size();

	CReaderBuffer readerBuffer;
	boost::ptr_vector<CDocumentBuffer> compressDocBuffers, tokenizeDocBuffers;
	CDocumentBuffer indexDocBuffer;
	
	for (uint32 i = 0; i < nrOfParsers; ++i)
	{
		compressDocBuffers.push_back(new CDocumentBuffer);
		tokenizeDocBuffers.push_back(new CDocumentBuffer);
		
		parser_threads.create_thread(boost::bind(&CBuilder::ParseFiles, this, mScriptName, &readerBuffer, &tokenizeDocBuffers.back()));
		tokenize_threads.create_thread(boost::bind(&CBuilder::TokenizeDoc, this, &tokenizeDocBuffers.back(), &compressDocBuffers.back()));
		compress_threads.create_thread(boost::bind(&CBuilder::CompressDoc, this, &compressDocBuffers.back(), &indexDocBuffer));
	}
	
	boost::thread indexDocThread(boost::bind(&CBuilder::IndexDoc, this, &indexDocBuffer, nrOfParsers));

	// show some progress if needed
	boost::thread* progress = nil;
	if (inShowProgress)
		progress = new boost::thread(boost::bind(&CBuilder::Progress, this));
	
	for (vector<fs::path>::iterator file = rawFiles.begin(); file != rawFiles.end(); ++file)
	{
		++mCurrentFile;
		
		mLastRawFile = file->leaf();
		
		CReaderPtr reader(CReader::CreateReader(*file));
		readerBuffer.Put(reader);
	}
	
	readerBuffer.Put(CReader::sEnd);
	
	parser_threads.join_all();
	compress_threads.join_all();
	indexDocThread.join();

	if (inShowProgress)
	{
		mParsingDone = true;
		progress->join();
		delete progress;
	}
	
	mStartTime = system_time();
	mLastUpdate = 0;

	if (VERBOSE > 1)
		cout << "done" << endl;
	
	mDatabank->Finish(true, false);
	delete mDatabank;
	mDatabank = nil;

	if (inShowProgress)
		cout << endl;
	
	if (mSafe != nil)
		mSafe->Commit();
}

static void GetSize(
	int64		inSize,
	uint32&		outSize,
	char&		outLetter)
{
	if (inSize > 1024LL * 1024 * 1024 * 1024)
	{
		outSize = static_cast<uint32>(inSize / (1024LL * 1024 * 1024 * 1024));
		outLetter = 'T';
	}
	else if (inSize > 1024LL * 1024 * 1024)
	{
		outSize = static_cast<uint32>(inSize / (1024LL * 1024 * 1024));
		outLetter = 'G';
	}
	else if (inSize > 1024LL * 1024)
	{
		outSize = static_cast<uint32>(inSize / (1024LL * 1024));
		outLetter = 'M';
	}
	else if (inSize > 1024LL)
	{
		outSize = static_cast<uint32>(inSize / (1024LL));
		outLetter = 'K';
	}
	else
	{
		outSize = static_cast<uint32>(inSize);
		outLetter = 'B';
	}
}

static string FormatTime(
	int64		inTime)
{
	stringstream result;

	if (inTime >= 60 * 60 * 24)
	{
		uint32 days = inTime / (60 * 60 * 24);
		result << days << 'd';
		inTime %= 60 * 60 * 24;
	}

	if (inTime >= 60 * 60)
	{
		uint32 hours = inTime / (60 * 60);
		result << hours << 'h';
		inTime %= 60 * 60;
	}

	if (inTime >= 60)
	{
		uint32 min = inTime / 60;
		result << setw(2) << min << 'm';
		inTime %= 60;
	}
	
	result << setw(2) << inTime << 's';
	
	return result.str();
}

void CBuilder::Progress()
{
	//            00        10        20        30        40        50        60        70        80        
	char msg[] = "                                                                                ";
	
	for (;;)
	{
		sleep(5);
		
		int64 readData;
		CReader::GetStatistics(readData);

		double progress = 0;
		
		if (readData > 0 and mRawDataSize > 0)
			progress = double(readData) / mRawDataSize;
		
		uint32 size, totalSize, rawSize;
		char sizeLetter, totalSizeLetter, rawSizeLetter;
		GetSize(readData, size, sizeLetter);
		GetSize(mRawDataSize, totalSize, totalSizeLetter);
		GetSize(fProcessedRawText, rawSize, rawSizeLetter);
		
		memset(msg, ' ', 80);
		
		if (mLastRawFile.length() < 20)
			mLastRawFile.copy(msg, mLastRawFile.length());
		else
			snprintf(msg, sizeof(msg), "%17.17s...", mLastRawFile.c_str());
		
		double now = system_time();
		double elapsed = now - mStartTime;
		double eta = 0;
		if (progress > 0)
			eta = (elapsed / (0.75 * progress)) - (elapsed / 0.75);
		
		snprintf(msg + 20, sizeof(msg) - 20,
			" %d/%d - %3.d%c/%3.d%c [%3.d%%] docs: %d text: %d%c ",
			mCurrentFile, mFileCount,
			size, sizeLetter,
			totalSize, totalSizeLetter,
			static_cast<uint32>(100 * progress),
			fProcessedDocuments,
			rawSize, rawSizeLetter);

		cout << '\r' << msg;
		
		if (not mParsingDone)
		{
			cout << "eta: " << FormatTime(eta);
			cout.flush();
		}
		else
		{
			cout << "elapsed: " << FormatTime(elapsed) << endl;
			break;
		}
	}
}

void CBuilder::SetCreateIndexProgress(
	int64		inCurrentLexEntry,
	int64		inTotalLexEntries)
{
	if (not mShowProgress)
		return;
	
	CDBBuildProgressMixin::SetCreateIndexProgress(inCurrentLexEntry, inTotalLexEntries);
	
	double now = system_time();
	if (now > mLastUpdate + 1 or
		inCurrentLexEntry == 0 or
		inCurrentLexEntry == inTotalLexEntries)
	{
		mLastUpdate = now;
		
		if (fTotalLexEntries == 0)
		{
			cout << '\r' << "Flushing buffers";
			cout.flush();
		}
		else
		{
			double progress = static_cast<double>(fCurrentLexEntry) / fTotalLexEntries;

			double elapsed = now - mStartTime;
			double eta = (elapsed / (0.8 * progress)) - (elapsed / 0.8);
			
			char msg[80];
			
			snprintf(msg, sizeof(msg),
				"Building index %Ld/%Ld [%3.d%%] ",
				fCurrentLexEntry, fTotalLexEntries,
				static_cast<uint32>(100 * progress));
			
			if (inCurrentLexEntry < inTotalLexEntries)
			{
				cout << '\r' << msg << "eta: " << FormatTime(eta);
				cout.flush();
			}
			else
				cout << '\r' << msg << "elapsed: " << FormatTime(elapsed) << endl;
		}
	}
}

void CBuilder::SetWritingIndexProgress(
	uint32		inCurrentKey,
	uint32		inKeyCount,
	const char*	inIndexName)
{
	if (mShowProgress)
	{
		CDBBuildProgressMixin::SetWritingIndexProgress(inCurrentKey, inKeyCount, inIndexName);
		cout << endl << "Writing index " << inIndexName;
		cout.flush();
	}
}

void CBuilder::CollectRawFiles(
	vector<fs::path>&	outRawFiles)
{
	mParser->CollectRawFiles(outRawFiles);
	
	mRawDataSize = 0;
	for (vector<fs::path>::iterator file = outRawFiles.begin(); file != outRawFiles.end(); ++file)
		mRawDataSize += fs::file_size(*file);
}

void CBuilder::ParseFiles(
	const string&		inScriptName,
	CReaderBuffer*		inReaderBuffer,
	CDocumentBuffer*	inCompressDocBuffer)
{
	CParser parser(mDatabankName, inScriptName, mRawDir.string());
	
	CParser::CDocCallback callBack =
		boost::bind(&CBuilder::ProcessDocument, this, _1, _2);

	CReaderPtr next;
	while ((next = inReaderBuffer->Get()) != CReader::sEnd)
		parser.Parse(*next, inCompressDocBuffer, callBack);
	
	inReaderBuffer->Put(CReader::sEnd);
	inCompressDocBuffer->Put(CDocument::sEnd);
}

void CBuilder::ProcessDocument(
	CDocumentPtr		inDocument,
	void*				inUserData)
{
	CDocumentBuffer* tokenizeDocBuffer = reinterpret_cast<CDocumentBuffer*>(inUserData);
	
	tokenizeDocBuffer->Put(inDocument);
}

void CBuilder::TokenizeDoc(
	CDocumentBuffer*	inTokenizeDocBuffer,
	CDocumentBuffer*	inCompressDocBuffer)
{
	CDocumentPtr next;
	
	while ((next = inTokenizeDocBuffer->Get()) != CDocument::sEnd)
	{
		next->TokenizeText(mLexicon, mLastStopWord);

		inCompressDocBuffer->Put(next);
	}
	
	inCompressDocBuffer->Put(CDocument::sEnd);
}
	
void CBuilder::CompressDoc(
	CDocumentBuffer*	inCompressDocBuffer,
	CDocumentBuffer*	inIndexDocBuffer)
{
	CDocumentPtr next;
	auto_ptr<CCompressor> compressor(mCompressorFactory->CreateCompressor());
	
	while ((next = inCompressDocBuffer->Get()) != CDocument::sEnd)
	{
		next->Compress(mMeta, *compressor);

		inIndexDocBuffer->Put(next);
	}
	
	inCompressDocBuffer->Put(CDocument::sEnd);
	inIndexDocBuffer->Put(CDocument::sEnd);
}

void CBuilder::IndexDoc(
	CDocumentBuffer*	inIndexDocBuffer,
	int32				inNrOfParsers)
{
	for (;;)
	{
		CDocumentPtr next = inIndexDocBuffer->Get();

		if (next == CDocument::sEnd)
		{
			if (--inNrOfParsers <= 0)
				break;

			continue;
		}

		mDatabank->StoreDocument(*next);

		const CDocument::CTokenMap& tokenData = next->GetTokenMap();

		for (CDocument::CTokenMap::const_iterator d = tokenData.begin(); d != tokenData.end(); ++d)
		{
			vector<uint32> tokens;
			tokens.reserve(d->tokens.size());
			
			vector<CDocument::CTokenData>::const_iterator t;
			for (t = d->tokens.begin(); t != d->tokens.end(); ++t)
				tokens.push_back(t->global_token);
			
			mDatabank->IndexTokens(d->index_name, d->index_kind, tokens);
		}

		mDatabank->FlushDocument();
	}
}

void CBuilder::ReadStopWords()
{
	set<string> stopwords;
	
	ifstream file("stop.txt");
	if (file.is_open())
	{
		while (not file.eof())
		{
			string line;
			getline(file, line);
			
			string::size_type s = line.find_first_of(" \t");
			if (s != string::npos)
				line.erase(s, string::npos);
			
			if (line.length() == 0)
				continue;
			
			stopwords.insert(line);
			mLastStopWord = mLexicon.Store(line);
		}
	}
	else
		THROW(("stop words file could not be opened"));
	
	mDatabank->SetStopWords(stopwords);
}

// ------------------------------------------------------------------

void usage()
{
	cout << "usage: mrs-build -d databank [-s script] [-a #threads] [-p] [-v]" << endl
		 << "    -d databank       The databank to build" << endl
		 << "    -s script         The parser script to use, default is same as databank" << endl
		 << "    -a #threads       The number of threads (pipelines) to use" << endl
		 << "    -S                Don't try to load the stopwords file (stop.txt)" << endl
		 << "    -p                Show progress information" << endl
		 << "    -v                Verbose output, can be specified multiple times"
		 << endl;
	exit(1);
}

void error(const char* msg, ...)
{
	char b[1024];
	
	va_list vl;
	va_start(vl, msg);
	vsnprintf(b, sizeof(b), msg, vl);
	va_end(vl);

	cerr << "mrs-build fatal error: " << b << endl;
	exit(1);
}

int main(int argc, char* const argv[])
{
	string script, databank;
	bool progress = false, readStopWords = true;
	int32 nrOfPipelines = 4;
	
	int c;
	
	while ((c = getopt(argc, argv, "s:d:vpa:S")) != -1)
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
			
			case 'p':
				progress = true;
				break;

			case 'a':
				nrOfPipelines = atoi(optarg);
				break;
			
			case 'S':
				readStopWords = false;
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
		
		if (readStopWords)
			builder.ReadStopWords();
		
		builder.Run(nrOfPipelines, progress);
	}
	catch (exception& e)
	{
		cerr << "mrs-build failed with exception:" << endl << e.what() << endl;
		exit(1);
	}
	catch (...)
	{
		cerr << "mrs-build failed with unhandled exception" << endl;
		exit(1);
	}
	
	return 0;
}
