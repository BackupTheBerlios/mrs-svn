#include "HLib.h"

#include <iostream>
#include <boost/filesystem.hpp>

#include "MRSInterface.h"
#include "WDatabankTable.h"
#include "WConfig.h"

using namespace std;
namespace fs = boost::filesystem;

//WSDatabankTable& WSDatabankTable::Instance()
//{
//	static WSDatabankTable sInstance;
//	return sInstance;
//}

WSDatabankTable::WSDatabankTable(
	const DBInfoVector&		inDbInfo)
{
	for (DBInfoVector::const_iterator dbi = inDbInfo.begin(); dbi != inDbInfo.end(); ++dbi)
	{
		fs::path f = gDataDir / (dbi->name + ".cmp");
		
		if (not fs::exists(f))
			continue;
			
		try
		{
			MDatabankPtr db(new MDatabank(f.string()));
			db->PrefetchDocWeights("__ALL_TEXT__");
			mDBs[dbi->name].mDB = db;
			mDBs[dbi->name].fasta = dbi->fasta;
			mDBs[dbi->name].blast = db->ContainsBlastIndex() or dbi->blast;
			mDBs[dbi->name].ignore = dbi->ignore_in_all;
		}
		catch (exception& e)
		{
			continue;
		}
	}
}

MDatabankPtr WSDatabankTable::operator[](const string& inCode)
{
	boost::mutex::scoped_lock lock(mLock);

	DBTable::iterator tbl = mDBs.find(inCode);
	
	if (tbl == mDBs.end())
		throw string("Databank not found: " + inCode).c_str();

	return tbl->second.mDB;
}

bool WSDatabankTable::Ignore(const string& inCode)
{
	boost::mutex::scoped_lock lock(mLock);

	return mDBs.find(inCode) == mDBs.end() or mDBs[inCode].ignore;
}

void WSDatabankTable::ReloadDbs()
{
	boost::mutex::scoped_lock lock(mLock);

	mDBs.clear();
	
	cout << endl;
	
	DBInfoVector dbInfo;
	
	if (gConfig != nil and gConfig->GetSetting("/mrs-config/dbs/db", dbInfo))
	{
		for (DBInfoVector::iterator dbi = dbInfo.begin(); dbi != dbInfo.end(); ++dbi)
		{
			fs::path f = gDataDir / (dbi->name + ".cmp");
				
			cout << "Loading " << dbi->name << " from " << f.string() << " ..."; cout.flush();
				
			try
			{
				MDatabankPtr db(new MDatabank(f.string()));
				db->PrefetchDocWeights("__ALL_TEXT__");
				mDBs[dbi->name].mDB = db;
				mDBs[dbi->name].fasta = dbi->fasta;
				mDBs[dbi->name].blast = db->ContainsBlastIndex() or dbi->blast;
				mDBs[dbi->name].ignore = dbi->ignore_in_all;
			}
			catch (exception& e)
			{
				cout << " failed" << endl;
				continue;
			}
			
			cout << " done" << endl;
		}
	}
	else
	{
		fs::directory_iterator end;
		for (fs::directory_iterator fi(gDataDir); fi != end; ++fi)
		{
			if (is_directory(*fi))
				continue;
			
			string name = fi->leaf();
			
			if (name.length() <= 4 or name.substr(name.length() - 4) != ".cmp")
				continue;
			
			name.erase(name.length() - 4);
	
			cout << "Loading " << name << " from " << fi->string() << " ..."; cout.flush();
			
			try
			{
				MDatabankPtr db(new MDatabank(fi->string()));
				db->PrefetchDocWeights("__ALL_TEXT__");
				mDBs[name].mDB = db;
				mDBs[name].blast = db->ContainsBlastIndex();
				mDBs[name].fasta = false;
//				mDBs[dbi->name].fasta = dbi->fasta;
//				mDBs[dbi->name].blast = dbi->blast;
			}
			catch (exception& e)
			{
				cout << " failed" << endl;
				continue;
			}
			
			cout << " done" << endl;
		}
	}
}
