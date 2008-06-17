#include "MRS.h"

#include "HError.h"
#include "HStream.h"
#include "HUrl.h"

#include <magic.h>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>
#include <zlib.h>
#include <fcntl.h>
#include <cerrno>
#include <iostream>
#include <list>
#include <algorithm>

#include "CReader.h"

using namespace std;
namespace fs = boost::filesystem;

// ------------------------------------------------------------------
//
//  libmagic support
//

class CLibMagic
{
  public:
	static CLibMagic&	Instance();
	
	bool				IsText(
							const fs::path&	inPath);

	string				GetMagic(
							const fs::path&	inPath);
	
  private:

						CLibMagic();
						~CLibMagic();

	magic_t				mCookie;
};

CLibMagic::CLibMagic()
{
	int flags = MAGIC_MIME | MAGIC_SYMLINK;
	
	mCookie = magic_open(flags);
	
	if (mCookie != nil)
		magic_load(mCookie, nil);
}

CLibMagic::~CLibMagic()
{
	magic_close(mCookie);
}

CLibMagic& CLibMagic::Instance()
{
	static CLibMagic sInstance;
	return sInstance;
}

bool CLibMagic::IsText(
	const fs::path&	inPath)
{
	bool result = false;
	const char* t;
	
	if (mCookie != nil and (t = magic_file(mCookie, inPath.string().c_str())) != nil)
		result = strncmp(t, "text/", 5) == 0;
	
	return result;
}

string CLibMagic::GetMagic(
	const fs::path&	inPath)
{
	const char* t;
	string result;
	
	if (mCookie != nil and (t = magic_file(mCookie, inPath.string().c_str())) != nil)
		result = t;
	
	return result;
}

// ------------------------------------------------------------------
//
//	CReaderImp
//

struct CReaderImp
{
					CReaderImp(
						const fs::path&		inFile);

	virtual			~CReaderImp();

	virtual bool	GetLine(
						string&				outLine) = 0;
	
	virtual bool	Eof() const = 0;
	
	static int64	GetReadData();

	fs::path		mFile;
	int64			mReadData;

	static boost::mutex			sListMutex;
	static list<CReaderImp*>	sList;
	static int64				sFinishedData;
};

boost::mutex		CReaderImp::sListMutex;
list<CReaderImp*>	CReaderImp::sList;
int64				CReaderImp::sFinishedData;

CReaderImp::CReaderImp(
	const fs::path&		inFile)
	: mFile(inFile)
	, mReadData(0)
{
	boost::mutex::scoped_lock	 lock(sListMutex);
	sList.push_back(this);
}

CReaderImp::~CReaderImp()
{
	boost::mutex::scoped_lock	 lock(sListMutex);
	
	sFinishedData += mReadData;
	sList.erase(remove(sList.begin(), sList.end(), this), sList.end());
}

int64 CReaderImp::GetReadData()
{
	boost::mutex::scoped_lock	 lock(sListMutex);

	int64 result = sFinishedData;
	for (list<CReaderImp*>::iterator imp = sList.begin(); imp != sList.end(); ++imp)
		result += (*imp)->mReadData;

	return result;
}

struct CTextReaderImpl : public CReaderImp
{
					CTextReaderImpl(
						const fs::path&		inFile);
	
	virtual bool	GetLine(
						string&				outLine);
	
	virtual bool	Eof() const;
	
	fs::ifstream	mFileStream;
};

CTextReaderImpl::CTextReaderImpl(
	const fs::path&		inFile)
	: CReaderImp(inFile)
	, mFileStream(inFile)
{
	if (not mFileStream.is_open())
		THROW(("Failed to open file %s", inFile.string().c_str()));
}
	
bool CTextReaderImpl::GetLine(
	string&				outLine)
{
	getline(mFileStream, outLine);
	outLine += "\n";
	
	mReadData += outLine.length();
	
	return not mFileStream.eof();
}
	
bool CTextReaderImpl::Eof() const
{
	return mFileStream.eof();
}

struct CGZipReaderImpl : public CReaderImp
{
	enum {
		kBufferSize = 10240
	};
					CGZipReaderImpl(
						const fs::path&		inFile);
	
					~CGZipReaderImpl();
	
	virtual bool	GetLine(
						string&				outLine);
	
	virtual bool	Eof() const;
	
	HFileStream		mFileStream;
	int64			mRemaining;
	z_stream_s		mZStream;
	uint8			mSrcBuffer[kBufferSize];
	uint8			mDstBuffer[kBufferSize];
	uint32			mDstPtr, mDstSize;
	bool			mEOF;
};

CGZipReaderImpl::CGZipReaderImpl(
	const fs::path&		inFile)
	: CReaderImp(inFile)
	, mFileStream(HUrl(inFile.string()), O_RDONLY)
	, mRemaining(mFileStream.Size())
	, mDstPtr(0)
	, mDstSize(0)
	, mEOF(false)
{
	memset(&mZStream, 0, sizeof(mZStream));
	
	int err = inflateInit2(&mZStream, 47);
	if (err != Z_OK)
		THROW(("Error initializing zlib: %s", mZStream.msg));
}

CGZipReaderImpl::~CGZipReaderImpl()
{
	inflateEnd(&mZStream);
}

bool CGZipReaderImpl::GetLine(
	string&				outLine)
{
	bool result = false;

	for (;;)
	{
		bool gotNewLine = false;
		
		while (mDstPtr < mDstSize)
		{
			result = true;
			
			char ch = mDstBuffer[mDstPtr];
			
			++mDstPtr;
			
			outLine += ch;
			
			if (ch == '\n')
			{
				gotNewLine = true;
				break;
			}
		}
		
		if (gotNewLine)
			break;
		
		mZStream.next_out = mDstBuffer;
		mZStream.avail_out = kBufferSize;

		// shift bytes in src buffer to beginning
		if (mZStream.next_in > mSrcBuffer and mZStream.avail_in > 0)
			memmove(mSrcBuffer, mZStream.next_in, mZStream.avail_in);
		mZStream.next_in = mSrcBuffer;
		
		int flush = 0;
		if (mRemaining > 0)
		{
			uint32 size = kBufferSize - mZStream.avail_in;
			if (size > mRemaining)
				size = static_cast<uint32>(mRemaining);
			mRemaining -= size;
			
			mFileStream.Read(mSrcBuffer + mZStream.avail_in, size);
			mZStream.avail_in += size;
		}
		else
			flush = Z_SYNC_FLUSH;
		
		int err = inflate(&mZStream, flush);
		
		if (err == Z_STREAM_END)
		{
			mEOF = true;
			break;
		}
		
		if (err < Z_OK)
			THROW(("Decompression error: %s (%d)", mZStream.msg, err));

		mReadData = mFileStream.Tell();

		mDstSize = kBufferSize - mZStream.avail_out;
		mDstPtr = 0;
	}
	
	return result;
}
	
bool CGZipReaderImpl::Eof() const
{
	return mEOF;
}

// ------------------------------------------------------------------

CReaderPtr	CReader::sEnd;

CReader::CReader(
	CReaderImp*	inImpl)
	: mImpl(inImpl)
{
}

CReader::~CReader()
{
	delete mImpl;
}

CReader* CReader::CreateReader(
	fs::path&	inFile)
{
	CLibMagic& magic = CLibMagic::Instance();
	CReader* result = nil;
	
	string name = inFile.leaf();
	if (name.rfind(".gz") == name.length() - 3)
	{
		result = new CReader(new CGZipReaderImpl(inFile));
	}
	else
	{
		string mimetype = magic.GetMagic(inFile);
		
		if (mimetype.substr(0, 5) == "text/")
			result = new CReader(new CTextReaderImpl(inFile));
		else if (mimetype == "application/x-gzip")
			result = new CReader(new CGZipReaderImpl(inFile));
		else if (mimetype == "application/octet-stream")
		{
			cerr << endl
				 << "Unknown file type for file " << inFile.leaf()
				 << ", falling back to text mode" << endl;
			result = new CReader(new CTextReaderImpl(inFile));
		}
		else
			THROW(("Unsupported file type '%s' for file %s",
				mimetype.c_str(),
				inFile.string().c_str()));
	}
	
	return result;
}

bool CReader::GetLine(
	string&		outLine)
{
	return mImpl->GetLine(outLine);
}

bool CReader::Eof()
{
	return mImpl->Eof();
}

void CReader::GetStatistics(
	int64&				outReadRawData)
{
	outReadRawData = CReaderImp::GetReadData();
}

