/*	$Id: CDatabank.cpp 28 2006-04-23 12:31:30Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#include <map>
#include <sstream>
#include <cstdarg>
#include <sys/stat.h>

#include <boost/shared_ptr.hpp>

#include "soapH.h"
#include "mrsws.nsmap"
#include "MRSInterface.h"
#include "WConfig.h"
#include "WError.h"
#include "WFormat.h"

using namespace std;

vector<DbInfo>	gDbInfo;
extern double system_time();

typedef boost::shared_ptr<MDatabank>	MDatabankPtr;

class WSDatabankTable
{
  public:
	static WSDatabankTable&	Instance();

	MDatabankPtr		operator[](const string& inCode);
	string				GetFormatter(const string& inCode);
	
	void				ReloadDbs();
	
  private:

	struct DBInfo
	{
		MDatabankPtr	mDb;
		string			mFormat;
		
						DBInfo();
						DBInfo(MDatabankPtr inDb, const string& inId);
						DBInfo(const DBInfo& inOther);

		bool			Valid();
	};

	typedef map<string,DBInfo>	DBTable;
	
	DBTable		mDBs;
};

WSDatabankTable& WSDatabankTable::Instance()
{
	static WSDatabankTable sInstance;
	return sInstance;
}

WSDatabankTable::DBInfo::DBInfo()
{
}

WSDatabankTable::DBInfo::DBInfo(const DBInfo& inOther)
	: mDb(inOther.mDb)
{
}

WSDatabankTable::DBInfo::DBInfo(
	MDatabankPtr		inDb,
	const string&		inId)
	: mDb(inDb)
{
	mFormat = "default";
	
	for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
	{
		if (dbi->id == inId)
		{
			mFormat = dbi->filter;
			break;
		}
	}
}

bool WSDatabankTable::DBInfo::Valid()
{
	return mDb.get() != NULL and mDb->IsUpToDate();
}

MDatabankPtr WSDatabankTable::operator[](const string& inCode)
{
	if (mDBs.find(inCode) == mDBs.end() or not mDBs[inCode].Valid())
	{
		MDatabankPtr db(new MDatabank(inCode));
		mDBs[inCode] = DBInfo(db, inCode);
	}
	
	return mDBs[inCode].mDb;
}

inline string WSDatabankTable::GetFormatter(const string& inCode)
{
	return mDBs[inCode].mFormat;
}

void WSDatabankTable::ReloadDbs()
{
	mDBs.clear();
	
	cout << endl;
	
	for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
	{
		if (dbi->id == "all")
			continue;
		
		cout << "Loading " << dbi->id << "..."; cout.flush();
		
		try
		{
			MDatabankPtr db(new MDatabank(dbi->id));
			mDBs[dbi->id] = DBInfo(db, dbi->id);
		}
		catch (exception& e)
		{
			cout << " failed" << endl;
			continue;
		}
		
		cout << " done" << endl;
	}
}

void GetDatabankInfo(const string& inDb, ns__DatabankInfo& outInfo)
{
	bool found = false;

	for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
	{
		if (dbi->id == inDb)
		{
			outInfo.id = inDb;
			outInfo.name = dbi->name;
			outInfo.filter = dbi->filter;
			outInfo.url = dbi->url;
			outInfo.parser = dbi->parser;
			outInfo.blastable = (dbi->blast != 0);
			
			outInfo.files.clear();
			
			char* first = const_cast<char*>(inDb.c_str());
			char* last = NULL;
			const char* dbn;
			
			while ((dbn = strtok_r(first, "+|", &last)) != NULL)
			{
				first = NULL;

				MDatabankPtr db = WSDatabankTable::Instance()[dbn];
				
				ns__FileInfo fi;
				
				fi.id = dbn;
				fi.uuid = db->GetUUID();
				fi.version = db->GetVersion();
				fi.path = db->GetFilePath();
				fi.entries = db->Count();
				fi.raw_data_size = db->GetRawDataSize();
				
				struct stat sb;
				string path = fi.path;
				if (path.substr(0, 7) == "file://")
					path.erase(0, 7);
				
				if (stat(path.c_str(), &sb) == 0)
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
			
			found = true;
			break;
		}
	}
	
	if (not found)
		THROW(("db %s not found", inDb.c_str()));
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetDatabankInfo(
	struct soap*						soap,
	string								db,
	vector<struct ns__DatabankInfo>&	info)
{
	int result = SOAP_OK;
	
	try
	{
		if (db == "all")
		{
			// how inefficient...
			
			for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
			{
				if (dbi->id == "all")
					continue;
				
				try
				{
					ns__DatabankInfo inf;
					GetDatabankInfo(dbi->id, inf);
					info.push_back(inf);
				}
				catch (...)
				{
					cerr << endl << "Skipping db " << dbi->id << endl;
				}
			}
		}
		else
		{
			ns__DatabankInfo inf;
			GetDatabankInfo(db, inf);
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
	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		auto_ptr<MIndices> mrsIndices(mrsDb->Indices());
		
		if (not mrsIndices.get())
			THROW(("Databank %s has no indices", db.c_str()));
		
		for (auto_ptr<MIndex> index(mrsIndices->Next()); index.get() != NULL; index.reset(mrsIndices->Next()))
		{
			ns__Index ix;
			
			ix.id = index->Code();
			ix.description = ix.id;	// for now...
			
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

	const char* title = db->GetMetaData(inId, "title");
	string result;
	
	if (title == NULL or *title == 0)
	{
		title = db->Get(inId);
		if (title != NULL and *title != 0)
			result = WFormatTable::Instance().Format(
						WSDatabankTable::Instance().GetFormatter(inDb),
						"title", title, inId);
	}
	else
		result = title;
	
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
	int result = SOAP_OK;
	
	try
	{
		MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
		
		const char* data = NULL;
		
		switch (format)
		{
			case plain:
				data = mrsDb->Get(id);
				if (data == NULL)
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
				entry = data;
				break;
			
			case title:
				entry = GetTitle(db, id);
				break;
			
			case fasta:
			{
				data = mrsDb->Sequence(id, 0);
				if (data == NULL)
					THROW(("Sequence for entry %s not found in databank %s", id.c_str(), db.c_str()));
				
				string sequence;
				while (*data != 0)
				{
					sequence += *data++;
					if ((sequence.length() + 1) % 73 == 0)
						sequence += '\n';
				}
				
				const char* descData = mrsDb->GetMetaData(id, "title");
				string desc;
				
				if (descData != NULL)
					desc = descData;

				for (string::iterator c = desc.begin(); c != desc.end(); ++c)
					if (*c == '\n') *c = ' ';

				stringstream s;
				s << '>' << id << ' ' << desc << endl << sequence << endl;
				entry = s.str();
				break;
			}
			
			case html:
			{
				data = mrsDb->Get(id);
				if (data == NULL)
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
					
				entry = WFormatTable::Instance().Format(
					WSDatabankTable::Instance().GetFormatter(db), "html", data, id);
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
	MDatabankPtr mrsDb = WSDatabankTable::Instance()[db];
	auto_ptr<MQueryResults> r;
	
	int result = SOAP_OK;

	if (queryterms.size() > 0)
	{
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
			
		q->SetAllTermsRequired(alltermsrequired);
	
		for (vector<string>::iterator qw = queryterms.begin(); qw != queryterms.end(); ++qw)
			q->AddTerm(*qw, 1);
		
		auto_ptr<MBooleanQuery> m;
		
		if (booleanfilter.length())
			m.reset(mrsDb->BooleanQuery(booleanfilter));
	
		r.reset(q->Perform(m.get()));
	}
	else if (booleanfilter.length() > 0)
		r.reset(mrsDb->Find(booleanfilter, false));
	
	if (r.get() != NULL)
	{
		response.count = r->Count(true);
		
		const char* id;
		
		while (resultoffset-- > 0 and (id = r->Next()) != NULL)
			;
		
		while (maxresultcount-- > 0 and (id = r->Next()) != NULL)
		{
			ns__Hit h;
			
			h.db = db;
			h.id = id;
			h.score = r->Score();
			h.title = GetTitle(db, id);
			
			response.hits.push_back(h);
		}
	}
	else
		response.count = 0;

	return result;
}

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
	int result = SOAP_OK;
	
	try
	{
		for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
		{
			if (not dbi->in_all)
				continue;
			
			try
			{
				MDatabankPtr mrsDb = WSDatabankTable::Instance()[dbi->id];
				auto_ptr<MQueryResults> r;
				
				if (queryterms.size() > 0)
				{
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
						
					q->SetAllTermsRequired(alltermsrequired);
				
					for (vector<string>::iterator qw = queryterms.begin(); qw != queryterms.end(); ++qw)
						q->AddTerm(*qw, 1);
					
					auto_ptr<MBooleanQuery> m;
					
					if (booleanfilter.length())
						m.reset(mrsDb->BooleanQuery(booleanfilter));
		
					r.reset(q->Perform(m.get()));
				}
				else if (booleanfilter.length() > 0)
					r.reset(mrsDb->Find(booleanfilter, false));
				
				if (r.get() != NULL)
				{
					ns__FindAllResult fa;
					fa.db = dbi->id;
					fa.count = r->Count(true);
					response.push_back(fa);
				}
			}
			catch (...)
			{
				cerr << endl << "Skipping db " << dbi->id << endl;
			}
		}
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

int main()
{
	struct sigaction sa;
	
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
		
	struct soap soap;
	int m, s; // master and slave sockets
	soap_init(&soap);

//	soap_set_recv_logfile(&soap, "recv.log"); // append all messages received in /logs/recv/service12.log
//	soap_set_sent_logfile(&soap, "sent.log"); // append all messages sent in /logs/sent/service12.log
//	soap_set_test_logfile(&soap, "test.log"); // no file name: do not save debug messages

	m = soap_bind(&soap, "localhost", 8081, 100);
	if (m < 0)
		soap_print_fault(&soap, stderr);
	else
	{
		gQuit = false;
		gNeedReload = true;
		
		soap.accept_timeout = 1;	// timeout
		
		fprintf(stderr, "Socket connection successful: master socket = %d\n", m);
		for (int i = 1; ; ++i)
		{
			if (gQuit)
				break;
			
			if (gNeedReload)
			{
				string dataDir;
				ReadConfig("mrs.conf", dataDir, gDbInfo);
				
				if (dataDir.length() > 0 and dataDir[dataDir.length() - 1] != '/')
					dataDir += '/';
			
				setenv("MRS_DATA_DIR", dataDir.c_str(), 1);
				
				WSDatabankTable::Instance().ReloadDbs();
				
				gNeedReload = false;
			}
			
			s = soap_accept(&soap);
			
			if (s == SOAP_EOF)
				continue;
			
			if (s < 0)
			{
				soap_print_fault(&soap, stderr);
				break;
			}
			fprintf(stderr, "%d: accepted connection from IP=%ld.%ld.%ld.%ld socket=%d...", i,
				(soap.ip >> 24)&0xFF, (soap.ip >> 16)&0xFF, (soap.ip >> 8)&0xFF, soap.ip&0xFF, s);
			
			double start = system_time();
			
			if (soap_serve(&soap) != SOAP_OK) // process RPC request
				soap_print_fault(&soap, stderr); // print error
			
			double end = system_time();
			
			fprintf(stderr, " request served in %.3g seconds\n", end - start);
			soap_destroy(&soap); // clean up class instances
			soap_end(&soap); // clean up everything and close socket
		}
	}
	soap_done(&soap); // close master socket and detach environment
	
	cout << "Quit" << endl;
	
	return 0;
}

