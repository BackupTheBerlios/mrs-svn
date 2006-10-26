/*	$Id: CDatabank.cpp 28 2006-04-23 12:31:30Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#include <map>
#include <sstream>
#include <cstdarg>

#include "soapH.h"
#include "mrsws.nsmap"
#include "MRSInterface.h"
#include "WConfig.h"

using namespace std;

vector<DbInfo>	gDbInfo;
extern double system_time();

class ws_exception : public std::exception
{
  public:
						ws_exception(const char* msg, ...);
						
	virtual const char*	what() const throw()		{ return msg; }
	
  private:
	char				msg[1024];
};

ws_exception::ws_exception(const char* inMessage, ...)
{
	using namespace std;
	
	va_list vl;
	va_start(vl, inMessage);
	vsnprintf(msg, sizeof(msg), inMessage, vl);
	va_end(vl);
#ifndef NDEBUG
	cerr << msg << endl;
#endif
}

inline void __report_throw(const char* ex, const char* file, int line)
{
#ifndef NDEBUG
	cerr << "Exception " << ex << " in File: " << file << " line: " << line << endl;
#endif
}

#define THROW(e)		do { __report_throw(#e, __FILE__, __LINE__); throw ws_exception e; } while (false)

int main()
{
	string dataDir;
	ReadConfig("mrs.conf", dataDir, gDbInfo);
	
	if (dataDir.length() > 0 and dataDir[dataDir.length() - 1] != '/')
		dataDir += '/';

	setenv("MRS_DATA_DIR", dataDir.c_str(), 1);

	soap_serve(soap_new()); // use the remote method request dispatcher
	return 0;
//
//	struct soap soap;
//	int m, s; // master and slave sockets
//	soap_init(&soap);
//
//	soap_set_recv_logfile(&soap, "recv.log"); // append all messages received in /logs/recv/service12.log
//	soap_set_sent_logfile(&soap, "sent.log"); // append all messages sent in /logs/sent/service12.log
//	soap_set_test_logfile(&soap, "test.log"); // no file name: do not save debug messages
//
//	m = soap_bind(&soap, "localhost", 8081, 100);
//	if (m < 0)
//		soap_print_fault(&soap, stderr);
//	else
//	{
//		fprintf(stderr, "Socket connection successful: master socket = %d\n", m);
//		for (int i = 1; ; ++i)
//		{
//			s = soap_accept(&soap);
//			if (s < 0)
//			{
//				soap_print_fault(&soap, stderr);
//				break;
//			}
//			fprintf(stderr, "%d: accepted connection from IP=%ld.%ld.%ld.%ld socket=%d...", i,
//				(soap.ip >> 24)&0xFF, (soap.ip >> 16)&0xFF, (soap.ip >> 8)&0xFF, soap.ip&0xFF, s);
//			
//			double start = system_time();
//			
//			if (soap_serve(&soap) != SOAP_OK) // process RPC request
//				soap_print_fault(&soap, stderr); // print error
//			
//			double end = system_time();
//			
//			fprintf(stderr, " request served in %.3g seconds\n", end - start);
//			soap_destroy(&soap); // clean up class instances
//			soap_end(&soap); // clean up everything and close socket
//		}
//	}
//	soap_done(&soap); // close master socket and detach environment
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
			outInfo.blastable = dbi->blast;
			
			MDatabank db(inDb);
			
			outInfo.uuid = db.GetUUID();
			outInfo.entries = db.Count();
			outInfo.version = db.GetVersion();
			outInfo.last_update = db.GetFileDate();
			
			found = true;
			
			break;
		}
	}
	
	if (not found)
		THROW(("db %s not found", inDb.c_str()));
}

SOAP_FMAC5 int SOAP_FMAC6 ns__GetDatabankInfo(struct soap* soap, string db, vector<struct ns__DatabankInfo >&info)
{
	int result = SOAP_OK;
	
	try
	{
		ns__DatabankInfo inf;
		
		if (db == "all")
		{
			// how inefficient...
			
			for (vector<DbInfo>::iterator dbi = gDbInfo.begin(); dbi != gDbInfo.end(); ++dbi)
			{
				if (dbi->id == "all")
					continue;
				
				GetDatabankInfo(dbi->id, inf);
				info.push_back(inf);
			}
		}
		else
		{
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

SOAP_FMAC5 int SOAP_FMAC6 ns__GetEntry(struct soap* soap, string db,
	string id, enum ns__Format format, string &entry)
{
	int result = SOAP_OK;
	
	try
	{
		MDatabank mrsDb(db);
		
		const char* data = NULL;
		
		switch (format)
		{
			case plain:
				data = mrsDb.Get(id);
				if (data == NULL)
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
				entry = data;
				break;
			
			case title:
				data = mrsDb.GetMetaData(id, "title");
				if (data == NULL)
					THROW(("Title for entry %s not found in databank %s", id.c_str(), db.c_str()));
				entry = data;
				break;
			
			case fasta:
			{
				data = mrsDb.Sequence(id, 0);
				if (data == NULL)
					THROW(("Sequence for entry %s not found in databank %s", id.c_str(), db.c_str()));
				
				string sequence;
				while (*data != 0)
				{
					sequence += *data++;
					if ((sequence.length() + 1) % 73 == 0)
						sequence += '\n';
				}
				
				const char* descData = mrsDb.GetMetaData(id, "title");
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
			
			default:
				THROW(("Unsupported format in GetEntry"));
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred while trying to get entry",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6 ns__Find(struct soap*, string db, vector<string> queryterms,
	bool alltermsrequired, enum ns__Algorithm algorithm, int resultoffset, int maxresultcount,
	struct ns__FindResponse& response)
{
	MDatabank mrsDb(db);
	auto_ptr<MQueryResults> r;
	
	int result = SOAP_OK;

	if (queryterms.size() > 0)
	{
		auto_ptr<MRankedQuery> q(mrsDb.RankedQuery("__ALL_TEXT__"));
	
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
//		if (filter.length())
//			m.reset(mrsDb.BooleanQuery(filter));
	
		r.reset(q->Perform(m.get()));
	}
//	else if (filter.length() > 0)
//		r.reset(mrsDb.Find(filter, false));
	
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
			
			const char* title = mrsDb.GetMetaData(id, "title");
			if (title != NULL)
				h.title = title;
			
			response.hits.push_back(h);
		}
	}
	else
		response.count = 0;

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6 ns__FindSimilar(struct soap*, string db, string id,
	enum ns__Algorithm algorithm, int resultoffset, int maxresultcount,
	struct ns__FindResponse& result)
{
	return SOAP_ERR;
}
