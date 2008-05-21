#ifndef WDATABANKTABLE_H
#define WDATABANKTABLE_H

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <map>
#include "WConfig.h"

class MDatabank;
typedef boost::shared_ptr<MDatabank>				MDatabankPtr;

class WSDatabankTable
{
  public:
						WSDatabankTable(
							const DBInfoVector&		inDbInfo);

	struct WSDB
	{
		MDatabankPtr	mDB;
		bool			ignore;
		bool			fasta;
		bool			blast;
	};

	typedef std::map<std::string,WSDB>	DBTable;
	typedef DBTable::const_iterator		iterator;

//	static WSDatabankTable&	Instance();

	MDatabankPtr		operator[](
							const std::string&		inCode);
	
	void				ReloadDbs();

	bool				Ignore(
							const std::string&		inDb);
	bool				Fasta(
							const std::string&		inDb);
	bool				Blast(
							const std::string&		inDb);
	
	iterator			begin() const					{ return mDBs.begin(); }
	iterator			end() const						{ return mDBs.end(); }
	
  private:
	DBTable				mDBs;
	boost::mutex		mLock;
};

#endif
