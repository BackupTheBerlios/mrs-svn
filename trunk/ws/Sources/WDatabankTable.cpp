#include "MRS.h"

#include <iostream>
#include <boost/filesystem.hpp>

#include "CDatabank.h"

#include "WDatabankTable.h"
#include "WConfig.h"

using namespace std;
namespace fs = boost::filesystem;

// --------------------------------------------------------------------
//

typedef vector<pair<string,CDatabankPtr> >	CDatabankList;

struct WSDatabankTableImp
{
						WSDatabankTableImp(
							WDbFileInfoArray&	inInfo);
						
						~WSDatabankTableImp();

	CDatabankPtr		GetDatabank(
							const string&		inDatabank);

	CDatabankList		mDatabanks;
};

WSDatabankTableImp::WSDatabankTableImp(
	WDbFileInfoArray&	inInfo)
{
	for (WDbFileInfoArray::iterator db = inInfo.begin(); db != inInfo.end(); ++db)
	{
		try
		{
			if (not db->is_joined_db)
			{
				fs::path dbfile = gDataDir / db->files.front();
				if (not fs::exists(dbfile))
					cout << "Databank file " << dbfile.string() << " does not exist, skipping" << endl;
				else
				{
					cout << "Loading databank file ..." << dbfile.string();
					
					CDatabankPtr dbPtr(new CDatabank(dbfile));
					mDatabanks.push_back(make_pair(db->id, dbPtr));
					cout << " done" << endl;
				}
			}
			else
			{
				vector<CDatabankPtr> parts;
				
				for (vector<string>::iterator p = db->files.begin(); p != db->files.end(); ++p)
					parts.push_back(GetDatabank(*p));
				
				CDatabankPtr dbPtr(new CJoinedDatabank(parts));
				mDatabanks.push_back(make_pair(db->id, dbPtr));
			}
		}
		catch (exception& e)
		{
			cerr << " failed" << endl
				 << "Error loading databank: " << e.what() << endl;
		}
	}
}

CDatabankPtr WSDatabankTableImp::GetDatabank(
	const string&		inDatabank)
{
	CDatabankPtr result;
	for (CDatabankList::iterator db = mDatabanks.begin(); db != mDatabanks.end(); ++db)
	{
		if (db->first == inDatabank)
		{
			result = db->second;
			break;
		}
	}
	
	if (not result)
		THROW(("Databank unknown '%s'", inDatabank.c_str()));
	
	return result;
}

// --------------------------------------------------------------------
//

WSDatabankTable& WSDatabankTable::Instance()
{
	static WSDatabankTable sInstance;
	return sInstance;
}

WSDatabankTable::WSDatabankTable()
	: mImpl(nil)
{
	boost::mutex::scoped_lock lock(mMutex);

	ReloadDbs();
}

CDatabankPtr WSDatabankTable::operator[](
	const string&		inCode)
{
	boost::mutex::scoped_lock lock(mMutex);
	
	return mImpl->GetDatabank(inCode);
}

void WSDatabankTable::ReloadDbs()
{
	boost::mutex::scoped_lock lock(mMutex);

	delete mImpl;
	mImpl = nil;
	
	WDbFileInfoArray info;
	gConfig->GetDbFiles(info);
	
	mImpl = new WSDatabankTableImp(info);
}
