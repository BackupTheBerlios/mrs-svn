#include "MRS.h"

#include "HError.h"

#include <magic.h>
#include <boost/filesystem/fstream.hpp>
#include <zlib.h>
#include <fcntl.h>
#include <cerrno>

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
						const fs::path&		inFile)
						: mFile(inFile)
					{
					}

	virtual			~CReaderImp() {}

	virtual bool	GetLine(
						string&				outLine) = 0;
	
	virtual bool	Eof() const = 0;

	fs::path		mFile;
};

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
	return not mFileStream.eof();
}
	
bool CTextReaderImpl::Eof() const
{
	return mFileStream.eof();
}

struct CGZipReaderImpl : public CReaderImp
{
					CGZipReaderImpl(
						const fs::path&		inFile);
	
					~CGZipReaderImpl();
	
	virtual bool	GetLine(
						string&				outLine);
	
	virtual bool	Eof() const;
	
	gzFile			mGZFile;
};

CGZipReaderImpl::CGZipReaderImpl(
	const fs::path&		inFile)
	: CReaderImp(inFile)
{
	int fd = open(inFile.string().c_str(), O_RDONLY);
	if (fd < 0)
		THROW(("Error opening file %s: %s", inFile.string().c_str(), strerror(errno)));
	
	mGZFile = gzdopen(fd, "rb");
	if (mGZFile == nil)
		THROW(("Error initializing decompression library"));
}

CGZipReaderImpl::~CGZipReaderImpl()
{
	if (mGZFile != nil)
		gzclose(mGZFile);
}

bool CGZipReaderImpl::GetLine(
	string&				outLine)
{
	bool result = false;
	
	if (not gzeof(mGZFile))
	{
		char b[1024];
		char* r = gzgets(mGZFile, b, sizeof(b));
		if (r != nil)
		{
			outLine = r;
			result = true;
		}
	}
	
	return result;
}
	
bool CGZipReaderImpl::Eof() const
{
	return gzeof(mGZFile);
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
	
	string mimetype = magic.GetMagic(inFile);
	
	if (mimetype.substr(0, 5) == "text/")
		result = new CReader(new CTextReaderImpl(inFile));
	else if (mimetype == "application/x-gzip")
		result = new CReader(new CGZipReaderImpl(inFile));
	else
		THROW(("Unsupported file type '%s' for file %s",
			mimetype.c_str(),
			inFile.string().c_str()));
	
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

