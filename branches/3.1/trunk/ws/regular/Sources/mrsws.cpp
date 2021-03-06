/*	$Id: mrsws.cpp$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#include "HLib.h"

#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <iomanip>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "soapH.h"
#include "mrsws.nsmap"
#include "MRSInterface.h"
#include "WError.h"
#include "WFormat.h"
#include "WUtils.h"
#include "WConfig.h"
#include "WBuffer.h"

#define nil NULL

using namespace std;
namespace fs = boost::filesystem;

// default values for the directories used by mrsws, these can also be set from the Makefile

#ifndef MRS_DATA_DIR
#define MRS_DATA_DIR "/usr/local/data/mrs/"
#endif

#ifndef MRS_PARSER_DIR
#define MRS_PARSER_DIR "/usr/local/share/mrs/parser_scripts/"
#endif

#ifndef MRS_CONFIG_FILE
#define MRS_CONFIG_FILE "/usr/local/etc/mrs-config.xml"
#endif

#ifndef MRS_LOG_FILE
#define MRS_LOG_FILE "/var/log/mrsws-search.log"
#endif

#ifndef MRS_PID_FILE
#define MRS_PID_FILE "/var/run/mrsws-search.pid"
#endif

fs::path gDataDir(MRS_DATA_DIR, fs::native);
fs::path gParserDir(MRS_PARSER_DIR, fs::native);
fs::path gConfigFile(MRS_CONFIG_FILE, fs::native);
fs::path gLogFile(MRS_LOG_FILE, fs::native);

WConfigFile*	gConfig = nil;

extern double system_time();

typedef boost::shared_ptr<MDatabank>				MDatabankPtr;

class WSDatabankTable
{
  public:
	struct WSDB
	{
		MDatabankPtr	mDB;
		bool			ignore;
		bool			fasta;
		bool			blast;
	};

	typedef map<string,WSDB>			DBTable;
	typedef DBTable::const_iterator		iterator;

	static WSDatabankTable&	Instance();

	MDatabankPtr		operator[](const string& inCode);
	
	void				ReloadDbs();

	bool				Ignore(const string& inDb);
	bool				Fasta(const string& inDb);
	bool				Blast(const string& inDb);
	
	iterator			begin() const					{ return mDBs.begin(); }
	iterator			end() const						{ return mDBs.end(); }
	
  private:
	DBTable				mDBs;
	boost::mutex		mLock;
};

WSDatabankTable& WSDatabankTable::Instance()
{
	static WSDatabankTable sInstance;
	return sInstance;
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
			
			if (name.length() < 4 or name.substr(name.length() - 4) != ".cmp")
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

// --------------------------------------------------------------------
//
//	SOAP calls
// 

inline string Format(
	MDatabankPtr		inDb,
	const string&		inFormat,
	const string&		inData,
	const string&		inID)
{
	return WFormatTable::Format(inDb->GetScriptName(), inFormat, inData, inDb->GetCode(), inID);
}

void GetDatabankInfo(
	MDatabankPtr		inMrsDb,
	ns__DatabankInfo&	outInfo)
{
	outInfo.id =		inMrsDb->GetCode();
	outInfo.name =		inMrsDb->GetName();
	outInfo.script =	inMrsDb->GetScriptName();
	outInfo.url =		inMrsDb->GetInfoURL();
	outInfo.blastable = inMrsDb->ContainsBlastIndex();
	
	outInfo.files.clear();

// only one file left... (I used to parse the name tokenizing by + and | characters)
	{
		ns__FileInfo fi;
		
		fi.id =				inMrsDb->GetCode();
		fi.uuid =			inMrsDb->GetUUID();
		fi.version =		inMrsDb->GetVersion();
		fi.path =			inMrsDb->GetFilePath();
		fi.entries =		inMrsDb->Count();
		fi.raw_data_size =	inMrsDb->GetRawDataSize();
		
		struct stat sb;
		string path = fi.path;
		if (path.substr(0, 7) == "file://")
			path.erase(0, 7);
		
		if (stat(path.c_str(), &sb) == 0 and sb.st_mtime != 0)
		{
			fi.file_size = sb.st_size;
			
			struct tm tm;
			char b[1024];

			strftime(b, sizeof(b), "%Y-%m-%d %H:%M:%S",
				localtime_r(&sb.st_mtime, &tm));
			
			fi.modification_date = b;
		}
		
		outInfo.files.push_back(fi);
	}
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetDatabankInfo(
	struct soap*						soap,
	string								db,
	vector<struct ns__DatabankInfo>&	info)
{
	WLogger log(soap->ip, __func__);
	log << db;

	int result = SOAP_OK;
	
	WSDatabankTable& dbt = WSDatabankTable::Instance();
	
	try
	{
		if (db == "all")
		{
			for (WSDatabankTable::iterator dbi = dbt.begin(); dbi != dbt.end(); ++dbi)
			{
				try
				{
					ns__DatabankInfo inf;
					GetDatabankInfo(dbi->second.mDB, inf);
					info.push_back(inf);
				}
				catch (...)
				{
					cerr << endl << "Skipping db " << dbi->first << endl;
				}
			}
		}
		else
		{
			ns__DatabankInfo inf;
			GetDatabankInfo(dbt[db], inf);
			info.push_back(inf);
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while getting version information",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetIndices(
	struct soap*				soap,
	string						db,
	vector<struct ns__Index >&	indices)
{
	WLogger log(soap->ip, __func__);
	log << db;

	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		auto_ptr<MIndices> mrsIndices(mrsDb->Indices());
		
		if (not mrsIndices.get())
			THROW(("Databank %s has no indices", db.c_str()));
		
		string script = mrsDb->GetScriptName();
		
		for (auto_ptr<MIndex> index(mrsIndices->Next()); index.get() != NULL; index.reset(mrsIndices->Next()))
		{
			ns__Index ix;
			
			ix.id = index->Code();
			ix.description = WFormatTable::IndexName(script, ix.id);
			ix.count = index->Count();
			
			string t = index->Type();
			if (t == "text" or t == "wtxt")
				ix.type = FullText;
			else if (t == "valu")
				ix.type = Unique;
			else if (t == "date")
				ix.type = Date;
			else if (t == "nmbr")
				ix.type = Number;
			
			indices.push_back(ix);
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			(string("An error occurred while trying to get indices for db ") + db).c_str(),
			e.what());
	}

	return result;
}

string GetTitle(
	const string&		inDb,
	const string&		inId)
{
	MDatabankPtr db(WSDatabankTable::Instance()[inDb]);

	string result;
	
	if (not db->GetMetaData(inId, "title", result) and
		db->Get(inId, result))
	{
		result = Format(db, "title", result, inId);
	}
	
	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetEntry(
	struct soap*	soap,
	string			db,
	string			id,
	enum ns__Format	format,
	string&			entry)
{
	WLogger log(soap->ip, __func__);
	log << db << ':' << id;

	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		switch (format)
		{
			case plain:
				log << ':' << "plain";
				if (not mrsDb->Get(id, entry))
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
				break;
			
			case title:
				log << ':' << "title";
				entry = GetTitle(db, id);
				break;
			
			case fasta:
			{
				log << ':' << "fasta";

				string sequence;
				
				if (mrsDb->ContainsBlastIndex())
				{
					if (not mrsDb->Sequence(id, 0, sequence))
						THROW(("Sequence for entry %s not found in databank %s", id.c_str(), db.c_str()));
				}
				else
				{
					if (not mrsDb->Get(id, sequence))
						THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
						
					sequence = Format(mrsDb, "sequence", sequence, id);
				}
				
				if (sequence == "__not_implemented__")
					THROW(("Retrieving a sequence from databank %s is not supported yet", db.c_str()));
				
				string::size_type n = 0;
				while (n + 72 < sequence.length())
				{
					sequence.insert(sequence.begin() + n + 72, 1, '\n');
					n += 73;
				}
				
				string desc = GetTitle(db, id);
				for (string::iterator c = desc.begin(); c != desc.end(); ++c)
					if (*c == '\n') *c = ' ';

				stringstream s;
				s << '>' << id << ' ' << desc << endl << sequence << endl;
				entry = s.str();
				break;
			}
			
			case html:
			{
				log << ':' << "html";

				if (not mrsDb->Get(id, entry))
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
					
				entry = Format(mrsDb, "html", entry, id);
				break;
			}
			
			default:
				THROW(("Unsupported format in GetEntry"));
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			(string("An error occurred while trying to get entry ") + id + " from db " + db).c_str(),
			e.what());
	}

	return result;
}

auto_ptr<MQueryResults>
PerformSearch(
	MDatabankPtr				db,
	vector<string>				queryterms,
	enum ns__Algorithm			algorithm,
	bool						alltermsrequired,
	string						booleanfilter)
{
	auto_ptr<MQueryResults> result;
	
	vector<string>::iterator qt = queryterms.begin();
	while (qt != queryterms.end())
	{
		if (qt->find('*') != string::npos or qt->find('?') != string::npos)
		{
			booleanfilter += " ";
			booleanfilter += *qt;
			
			qt = queryterms.erase(qt);
		}
		else
			++qt;
	}
	
	if (queryterms.size() > 0)
	{
		// first determine the set of value indices, needed as a special case in ranked searches
		set<string> valueIndices;

		auto_ptr<MIndices> indices(db->Indices());
		MIndex* index;
		while ((index = indices->Next()) != NULL)
		{
			if (index->Type() == "valu")
				valueIndices.insert(index->Code());
			delete index;
		}

		auto_ptr<MRankedQuery> q(db->RankedQuery("__ALL_TEXT__"));
	
		switch (algorithm)
		{
			case Vector:
				q->SetAlgorithm("vector");
				break;
			
			case Dice:
				q->SetAlgorithm("dice");
				break;
			
			case Jaccard:
				q->SetAlgorithm("jaccard");
				break;
			
			default:
				THROW(("Unsupported search algorithm"));
				break;
		}
			
		q->SetAllTermsRequired(alltermsrequired);
		
		for (vector<string>::iterator qw = queryterms.begin(); qw != queryterms.end(); ++qw)
			q->AddTerm(*qw, 1);
		
		auto_ptr<MBooleanQuery> m;
		if (booleanfilter.length())
			m.reset(db->BooleanQuery(booleanfilter));

		result.reset(q->Perform(m.get()));
	}
	else if (booleanfilter.length() > 0)
		result.reset(db->Find(booleanfilter, false));
	
	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__Find(
	struct soap*				soap,
	string						db,
	vector<string>				queryterms,
	enum ns__Algorithm			algorithm,
	bool						alltermsrequired,
	string						booleanfilter,
	int							resultoffset,
	int							maxresultcount,
	struct ns__FindResponse&	response)
{
	WLogger log(soap->ip, __func__);

	log << db;

	stringstream s;
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(s, ","));
	log << s.str();
	
	if (booleanfilter.length())
		log << ':' << '[' << booleanfilter << ']';

	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		if (mrsDb.get() == nil)
			THROW(("Databank not found: %s", db.c_str()));
		
		auto_ptr<MQueryResults> r =
			PerformSearch(mrsDb, queryterms, algorithm, alltermsrequired, booleanfilter);
		
		if (r.get() != NULL)
		{
			const char* id;
			while (resultoffset-- > 0 and (id = r->Next()) != NULL)
				;
			
			while (maxresultcount-- > 0 and (id = r->Next()) != NULL)
			{
				ns__Hit h;
	
				h.id = id;
				h.score = r->Score();
				h.db = db;
				h.title = GetTitle(db, h.id);
				
				response.hits.push_back(h);
			}
	
			response.count = r->Count(true);
		}
		else
			response.count = 0;
	}
	catch (std::exception& e)
	{
		return soap_receiver_fault(soap,
			(string("An error occurred while trying do a search in db ") + db).c_str(),
			e.what());
	}

	return result;
}

struct SortOnFirstHitScore
{
	bool operator()(const ns__FindAllResult& a, const ns__FindAllResult& b) const
		{ return a.hits.front().score > b.hits.front().score; }
};

SOAP_FMAC5 int SOAP_FMAC6
ns__FindAll(
	struct soap*		soap,
	vector<string>		queryterms,
	enum ns__Algorithm	algorithm,
	bool				alltermsrequired,
	string				booleanfilter,
	vector<struct ns__FindAllResult>&
						response)
{
	WLogger log(soap->ip, __func__);

	stringstream s;
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(s, ","));
	log << s.str();
	
	if (booleanfilter.length())
		log << ':' << '[' << booleanfilter << ']';

	int result = SOAP_OK;
	
	WSDatabankTable& dbt = WSDatabankTable::Instance();
	
	try
	{
		for (WSDatabankTable::iterator dbi = dbt.begin(); dbi != dbt.end(); ++dbi)
		{
			if (dbt.Ignore(dbi->first))
				continue;

			auto_ptr<MQueryResults> r = PerformSearch(
				dbi->second.mDB, queryterms, algorithm, alltermsrequired, booleanfilter);

			if (r.get() != NULL)
			{
				ns__FindAllResult fa;

				fa.db = dbi->first;
				
				const char* id;
				int n = 5;
				
				while (n-- > 0 and (id = r->Next()) != NULL)
				{
					ns__Hit h;
		
					h.id = id;
					h.score = r->Score();
					h.db = fa.db;
					h.title = GetTitle(h.db, h.id);
					
					fa.hits.push_back(h);
				}

				fa.count = r->Count(true);
				response.push_back(fa);
			}
		}
		
		stable_sort(response.begin(), response.end(), SortOnFirstHitScore());
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while doing a FindAll",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__SpellCheck(
	struct soap*	soap,
	string			db,
	string			queryterm,
	vector<string>&	suggestions)
{
	WLogger log(soap->ip, __func__);
	log << db << ':' << queryterm;

	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		auto_ptr<MStringIterator> s(mrsDb->SuggestCorrection(queryterm));
		if (s.get() != NULL)
		{
			const char* sw;
			while ((sw = s->Next()) != NULL)
				suggestions.push_back(sw);
		}
	}
	catch (exception& e)
	{
		result = soap_receiver_fault(soap,
			"An error occurred while trying to get spelling suggestions",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__FindSimilar(
	struct soap*				soap,
	string						db,
	string						id,
	enum ns__Algorithm			algorithm,
	int							resultoffset,
	int							maxresultcount,
	struct ns__FindResponse&	response)
{
	WLogger log(soap->ip, __func__);
	log << db << ':' << id;

	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		string data;
		if (not mrsDb->Get(id, data))
			THROW(("Entry '%s' not found in '%d'", id.c_str(), db.c_str()));

		string entry = Format(mrsDb, "indexed", data, id);
		
		auto_ptr<MRankedQuery> q(mrsDb->RankedQuery("__ALL_TEXT__"));
	
		switch (algorithm)
		{
			case Vector:
				q->SetAlgorithm("vector");
				break;
			
			case Dice:
				q->SetAlgorithm("dice");
				break;
			
			case Jaccard:
				q->SetAlgorithm("jaccard");
				break;
			
			default:
				THROW(("Unsupported search algorithm"));
				break;
		}
		
		q->SetAllTermsRequired(false);
		q->AddTermsFromText(entry);

		auto_ptr<MQueryResults> r(q->Perform(NULL));

		if (r.get() != NULL)
		{
			const char* id;
			while (resultoffset-- > 0 and (id = r->Next()) != NULL)
				;
			
			while (maxresultcount-- > 0 and (id = r->Next()) != NULL)
			{
				ns__Hit h;
	
				h.id = id;
				h.score = r->Score();
				h.db = db;
				h.title = GetTitle(db, h.id);
				
				response.hits.push_back(h);
			}

			response.count = r->Count(true);
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			(string("An error occurred while trying to get entry ") + id + " from db " + db).c_str(),
			e.what());
	}

	return result;
}

//class CSearchSimilarThread : public CThread
//{
//  public:
//					CSearchSimilarThread(
//						string				dbName,
//						MDatabankPtr		db,
//						const string&		entry,
//						enum ns__Algorithm	algorithm);
//
//	auto_ptr<MQueryResults>
//					Results()				{ return fResults; }
//	string			Db()					{ return fDbName; }
//	
//  protected:
//	
//	virtual void	Run();
//
//  private:
//	auto_ptr<MQueryResults>
//						fResults;
//	string				fDbName;
//	MDatabankPtr		fDB;
//	const string&		fEntry;
//	enum ns__Algorithm	fAlgorithm;
//};
//
//CSearchSimilarThread::CSearchSimilarThread(
//	string				dbName,
//	MDatabankPtr		db,
//	const string&		entry,
//	enum ns__Algorithm	algorithm)
//	: fDbName(dbName)
//	, fDB(db)
//	, fEntry(entry)
//	, fAlgorithm(algorithm)
//{
//}
//
//void CSearchSimilarThread::Run()
//{
//	try
//	{
//		auto_ptr<MRankedQuery> q(fDB->RankedQuery("__ALL_TEXT__"));
//	
//		switch (fAlgorithm)
//		{
//			case Vector:
//				q->SetAlgorithm("vector");
//				break;
//			
//			case Dice:
//				q->SetAlgorithm("dice");
//				break;
//			
//			case Jaccard:
//				q->SetAlgorithm("jaccard");
//				break;
//			
//			default:
//				THROW(("Unsupported search algorithm"));
//				break;
//		}
//			
//		q->SetAllTermsRequired(false);
//		q->AddTermsFromText(fEntry);
//
//		fResults.reset(q->Perform(NULL));
//	}
//	catch (exception& e)
//	{
//		cout << "exception in CSearchSimilarThread::Run: " << e.what() << endl;
//	}
//	catch (...)
//	{
//		cout << "unknown exception in CSearchSimilarThread::Run" << endl;
//	}
//}

SOAP_FMAC5 int SOAP_FMAC6
ns__FindAllSimilar(
	struct soap*		soap,
	string				db,
	string				id,
	enum ns__Algorithm	algorithm,
	vector<struct ns__FindAllResult>&
						response)
{
	WLogger log(soap->ip, __func__);
	log << db << ':' << id;
	
	int result = SOAP_OK;
	
	WSDatabankTable& dbt = WSDatabankTable::Instance();
	
	try
	{
		MDatabankPtr mrsDb = dbt[db];
		
		string data;
		if (not mrsDb->Get(id, data))
			THROW(("Entry '%s' not found in '%d'", id.c_str(), db.c_str()));

		string entry = Format(mrsDb, "indexed", data, id);

		for (WSDatabankTable::iterator dbi = dbt.begin(); dbi != dbt.end(); ++dbi)
		{
			if (dbt.Ignore(dbi->first))
				continue;

			try
			{
				auto_ptr<MRankedQuery> q(dbi->second.mDB->RankedQuery("__ALL_TEXT__"));
			
				switch (algorithm)
				{
					case Vector:
						q->SetAlgorithm("vector");
						break;
					
					case Dice:
						q->SetAlgorithm("dice");
						break;
					
					case Jaccard:
						q->SetAlgorithm("jaccard");
						break;
					
					default:
						THROW(("Unsupported search algorithm"));
						break;
				}
					
				q->SetAllTermsRequired(false);
				q->AddTermsFromText(entry);
		
				auto_ptr<MQueryResults> r(q->Perform(NULL));

				if (r.get() != NULL)
				{
					ns__FindAllResult fa;
	
					fa.db = dbi->first;
					
					const char* id;
					int n = 5;
					
					while (n-- > 0 and (id = r->Next()) != NULL)
					{
						ns__Hit h;
			
						h.id = id;
						h.score = r->Score();
						h.db = fa.db;
						h.title = GetTitle(h.db, h.id);
						
						fa.hits.push_back(h);
					}
	
					fa.count = r->Count(true);
					response.push_back(fa);
				}
			}
			catch (...)
			{
				cerr << endl << "Skipping db " << dbi->first << endl;
			}
		}
		
		stable_sort(response.begin(), response.end(), SortOnFirstHitScore());
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while doing a FindAll",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__Count(
	struct soap*		soap,
	string				db,
	string				booleanquery,
	unsigned long&		response)
{
	WLogger log(soap->ip, __func__);
	log << db << ':' << booleanquery;
	
	int result = SOAP_OK;
	response = 0;
	
	try
	{
		WSDatabankTable& dbt = WSDatabankTable::Instance();
		
		MDatabankPtr mrsDb = dbt[db];
		auto_ptr<MQueryResults> r(mrsDb->Find(booleanquery));
		if (r.get() != nil)
			response = r->Count(true);
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while doing a Count",
			e.what());
	}

	return result;
}

struct CoTerm {
	string		term;
	uint32		count;
	float		idf;
	
	bool		operator<(const CoTerm& rhs) const
					{ return term < rhs.term; }
	
};

struct CoTermScoreGreater
{
	bool operator()(const CoTerm& a, const CoTerm& b) const
		{ return (a.idf * a.count) > (b.idf * b.count); }
};

typedef set<CoTerm> CoTermSet;

SOAP_FMAC5 int SOAP_FMAC6
ns__Cooccurrence(
	struct soap*		soap,
	string				db,
	vector<string>		ids,
	float				idf_cutoff,
	int					resultoffset,
	int					maxresultcount,
	vector<string>&		response)
{
	WLogger log(soap->ip, __func__);

	stringstream s;
	copy(ids.begin(), ids.end(), ostream_iterator<string>(s, ","));

	log << db << ':' << s.str();
	
	int result = SOAP_OK;
	
	if (idf_cutoff < 0)
		idf_cutoff = 0;
	
	try
	{
		WSDatabankTable& dbt = WSDatabankTable::Instance();
		
		MDatabankPtr mrsDb = dbt[db];

		boost::regex re("\\s");
		boost::regex re2("^\\d+(\\.\\d+)?$");	// numbers are not allowed...
		
		// build a list of terms, 
		CoTermSet r;
		
		set<string> idSet;
		copy(ids.begin(), ids.end(), inserter(idSet, idSet.begin()));

		for (vector<string>::iterator id = ids.begin(); id != ids.end(); ++id)
		{
			string data;
			if (not mrsDb->Get(*id, data))
				THROW(("Entry '%s' not found in '%d'", id->c_str(), db.c_str()));

			string entry = Format(mrsDb, "indexed", data, *id);

			set<string> u;
			boost::sregex_token_iterator i(entry.begin(), entry.end(), re, -1), j;

			for (; i != j; ++i)
			{
				string s = *i;
				if (idSet.count(*i) == 0 and not boost::regex_match(s, re2))
					u.insert(s);
			}

			for (set<string>::iterator i = u.begin(); i != u.end(); ++i)
			{
				CoTerm t = {};
				t.term = *i;
				
				pair<CoTermSet::iterator,bool> f = r.insert(t);
				const_cast<CoTerm&>(*f.first).count += 1;
			}
		}
		
		vector<CoTerm> terms;
		terms.reserve(r.size());

		auto_ptr<MIndex> index(mrsDb->Index("__ALL_TEXT__"));
		
		for (CoTermSet::iterator t = r.begin(); t != r.end(); ++t)
		{
			if (t->count > 1)
			{
				try
				{
					const_cast<CoTerm&>(*t).idf = index->GetIDF(t->term);
					
					if (t->idf > idf_cutoff)
						terms.push_back(*t);
				}
				catch (...) {}
			}
		}

		// do we have enough terms?
		if (static_cast<int32>(terms.size()) > maxresultcount - resultoffset)
		{
			sort(terms.begin(), terms.end(), CoTermScoreGreater());
			
			vector<CoTerm>::iterator t = terms.begin();
			int32 n = 0;
			
			while (n < resultoffset and t != terms.end())
			{
				++t;
				++n;
			}
			
			while (n < resultoffset + maxresultcount and t != terms.end())
			{
				response.push_back(t->term);
				++t;
				++n;
			}
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while doing a Count",
			e.what());
	}

	return result;
}

// --------------------------------------------------------------------
// 
//   multi threaded support
// 

typedef WBuffer<pair<int,int>,10>	Buffer;

void Process(
	Buffer*			inBuffer,
	struct soap*	inSoap)
{
	struct soap* soap = soap_copy(inSoap);

	for (;;)
	{
		try
		{
			pair<int,int> sip = inBuffer->Get();
			
			soap->socket = sip.first;
			soap->ip = sip.second;
			
			if (not soap_valid_socket(soap->socket))
				break;

			int err = soap_serve(soap);	 // process RPC request
			if (err != SOAP_OK and err != SOAP_EOF)
				soap_print_fault(soap, stderr); // print error

			soap_destroy(soap); // clean up class instances
			soap_end(soap); // clean up everything and close socket
		}
		catch (exception& e)
		{
			cout << endl << "Exception: " << e.what() << endl;
		}
		catch (...)
		{
			cout << endl << "Unknown exception" << endl;
		}
	}

	soap_done(soap); // close master socket and detach environment
	free(soap);
}

// --------------------------------------------------------------------
// 
//   main body
// 

// globals for communication with the outside world

bool	gNeedReload;
bool	gQuit;

void handler(int inSignal)
{
	int old_errno = errno;
	
	switch (inSignal)
	{
		case SIGINT:
			gQuit = true;
			break;
		
		case SIGHUP:
			gNeedReload = true;
			break;
		
		default:	// now what?
			break;
	}
	
	cout << "signal caught: " << inSignal << endl;
	
	errno = old_errno;
}

void usage()
{
	cout << "usage: mrsws [-d datadir] [-p parserdir] [[-a address] [-p port] | -i input] [-v]" << endl;
	cout << "    -d   data directory containing MRS files (default " << gDataDir.string() << ')' << endl;
	cout << "    -p   parser directory containing parser scripts (default " << gParserDir.string() << ')' << endl;
	cout << "    -a   address to bind to (default localhost)" << endl;
	cout << "    -p   port number to bind to (default 8081)" << endl;
	cout << "    -i   process command from input file and exit" << endl;
	cout << "    -f   stay in foreground, do not daemonize" << endl;
	cout << "    -v   be verbose" << endl;
	cout << endl;
	exit(1);
}

int main(int argc, const char* argv[])
{
	int c;
	string input_file, address = "localhost", config_file, user;
	short port = 8081, nrOfThreads = 2;
	bool daemon = true;
	
	while ((c = getopt(argc, const_cast<char**>(argv), "d:s:a:p:i:c:vf")) != -1)
	{
		switch (c)
		{
			case 'd':
				gDataDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 's':
				gParserDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'a':
				address = optarg;
				break;
			
			case 'p':
				port = atoi(optarg);
				break;
			
			case 'i':
				input_file = optarg;
				break;
			
			case 'c':
				gConfigFile = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'v':
				++VERBOSE;
				break;
			
			case 'f':
				daemon = false;
				break;
			
			default:
				usage();
				break;
		}
	}
	
	// check the parameters
	
	if (fs::exists(gConfigFile))
	{
		gConfig = new WConfigFile(gConfigFile.string().c_str());
		
		string s;

		// user ID to use
		gConfig->GetSetting("/mrs-config/user", user);
		
		if (gConfig->GetSetting("/mrs-config/datadir", s))
			gDataDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/scriptdir", s))
			gParserDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/search-ws/logfile", s))
			gLogFile = fs::system_complete(fs::path(s, fs::native));
	
		if (gConfig->GetSetting("/mrs-config/search-ws/address", s))
			address = s;
	
		if (gConfig->GetSetting("/mrs-config/search-ws/port", s))
			port = atoi(s.c_str());

		if (gConfig->GetSetting("/mrs-config/search-ws/threads", s))
			nrOfThreads = atoi(s.c_str());
	}
	else if (VERBOSE)
		cerr << "Configuration file " << gConfigFile.string() << " does not exist, ignoring" << endl;
	
	if (not fs::exists(gDataDir) or not fs::is_directory(gDataDir))
	{
		cerr << "Data directory " << gDataDir.string() << " is not a valid directory" << endl;
		exit(1);
	}
	
	setenv("MRS_DATA_DIR", gDataDir.string().c_str(), 1);

	if (not fs::exists(gParserDir) or not fs::is_directory(gParserDir))
	{
		cerr << "Parser directory " << gParserDir.string() << " is not a valid directory" << endl;
		exit(1);
	}

	WFormatTable::SetParserDir(gParserDir.string());
	
	if (input_file.length())
	{
		struct soap soap;
		
		soap_init(&soap);

		WSDatabankTable::Instance().ReloadDbs();
					
		soap_serve(&soap);
		soap_destroy(&soap);
		soap_end(&soap);
	}
	else
	{
		ofstream logFile;
		
		if (daemon)
			Daemonize(user, gLogFile.string(), MRS_PID_FILE, logFile);
		
		struct sigaction sa;
		
		sa.sa_handler = handler;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGHUP, &sa, NULL);
		sigaction(SIGPIPE, &sa, NULL);
		
		struct soap soap;
		int m, s; // master and slave sockets
		soap_init2(&soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE|SOAP_C_UTFSTRING);

//	soap_set_recv_logfile(&soap, "recv.log"); // append all messages received in /logs/recv/service12.log
//	soap_set_sent_logfile(&soap, "sent.log"); // append all messages sent in /logs/sent/service12.log
//	soap_set_test_logfile(&soap, "test.log"); // no file name: do not save debug messages

		if (VERBOSE)
			cout << "Binding address " << address << " port " << port << endl;

		// enable reuse of our address
		soap.bind_flags = SO_REUSEADDR;

		m = soap_bind(&soap, address.c_str(), port, 100);
		if (m < 0)
			soap_print_fault(&soap, stderr);
		else
		{
			gQuit = false;
			
			while (not gQuit)
			{
				WSDatabankTable::Instance().ReloadDbs();
				gNeedReload = false;

				Buffer buffer;
				boost::thread_group threads;
				for (int i = 0; i < nrOfThreads; ++i)
					threads.create_thread(boost::bind(Process, &buffer, &soap));
	
				for (;;)
				{
					if (gQuit or gNeedReload)
						break;
	
					soap.accept_timeout = 5;	// timeout
					soap.recv_timeout = 30;
					soap.send_timeout = 30;
					soap.max_keep_alive = 10;
					
					s = soap_accept(&soap);
					
					if (s != SOAP_EOF)
					{
						if (soap.errnum != 0)
							soap_print_fault(&soap, stderr);
						else
							buffer.Put(make_pair(s, soap.ip));
					}
				}
				
				for (int i = 0; i < nrOfThreads; ++i)
					buffer.Put(make_pair(SOAP_EOF, 0));
				
				threads.join_all();
			}
		}

		soap_done(&soap); // close master socket and detach environment
		
		cout << "Quit" << endl;
	}
	
	return 0;
}

