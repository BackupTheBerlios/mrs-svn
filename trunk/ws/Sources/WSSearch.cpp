#include "mrsws.h"

#include <set>
#include <boost/regex.hpp>

#include "HError.h"

#include "CDatabank.h"

#include "WSSearchNSH.h"
#include "WConfig.h"
#include "WSSearch.h"
#include "WFormat.h"
#include "WDatabankTable.h"

#include "WSSearchNS.nsmap"

using namespace std;

// --------------------------------------------------------------------
//
//	SOAP calls
// 

namespace WSSearchNS {

SOAP_FMAC5 int SOAP_FMAC6 ns__GetDatabankInfo(
	struct soap*				soap,
	string						db,
	vector<struct ns__DatabankInfo >&
								info)
{
	return WSSearch::GetServer(soap)->GetDatabankInfo(db, info);
}

int ns__GetEntry(
	struct soap*				soap,
	string						db,
	string						id,
	enum ns__Format				format,
	string&						entry)
{
	return WSSearch::GetServer(soap)->GetEntry(db, id, format, entry);
}

int ns__GetIndices(
	struct soap*				soap,
	string						db,
	vector<struct ns__Index>&	indices)
{
	return WSSearch::GetServer(soap)->GetIndices(db, indices);
}

int ns__Find(
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
	return WSSearch::GetServer(soap)->Find(db, queryterms, algorithm,
		alltermsrequired, booleanfilter, resultoffset, maxresultcount, response);
}

int ns__FindSimilar(
	struct soap*				soap,
	string						db,
	string						id,
	enum ns__Algorithm			algorithm,
	int							resultoffset,
	int							maxresultcount,
	struct ns__FindResponse&	response)
{
	return WSSearch::GetServer(soap)->FindSimilar(db, id, algorithm,
		resultoffset, maxresultcount, response);
}

int ns__FindAll(
	struct soap*				soap,
	vector<string>				queryterms,
	enum ns__Algorithm			algorithm,
	bool						alltermsrequired,
	string						booleanfilter,
	vector<struct ns__FindAllResult>&
								response)
{
	return WSSearch::GetServer(soap)->FindAll(queryterms, algorithm,
		alltermsrequired, booleanfilter, response);
}

int ns__FindAllSimilar(
	struct soap*				soap,
	string						db,
	string						id,
	enum ns__Algorithm			algorithm,
	vector<struct ns__FindAllResult>&
								response)
{
	return WSSearch::GetServer(soap)->FindAllSimilar(db, id, algorithm, response);
}

int ns__Count(
	struct soap*				soap,
	string						db,
	string						booleanquery,
	unsigned long&				response)
{
	return WSSearch::GetServer(soap)->Count(db, booleanquery, response);
}

int ns__Cooccurrence(
	struct soap*				soap,
	string						db,
	vector<string>				ids,
	float						idf_cutoff,
	int							resultoffset,
	int							maxresultcount,
	vector<string>&				terms)
{
	return WSSearch::GetServer(soap)->Cooccurrence(db, ids, idf_cutoff,
		resultoffset, maxresultcount, terms);
}

int ns__SpellCheck(
	struct soap*				soap,
	string						db,
	string						queryterm,
	vector<string>&				suggestions)
{
	return WSSearch::GetServer(soap)->SpellCheck(db, queryterm, suggestions);
}

}

// --------------------------------------------------------------------
//
//	Server implementation
// 

WSSearch::WSSearch(
	const string&	inAddress,
	uint16			inPortNr,
	vector<string>&	inDbs)
	: WServerT<WSSearch>(inAddress, inPortNr, WSSearchNS_namespaces)
	, mDBs(inDbs)
{
}

int WSSearch::GetDatabankInfo(
	string											db,
	vector<struct WSSearchNS::ns__DatabankInfo>&	info)
{
	Log() << db;

	WSDatabankTable& tbl = WSDatabankTable::Instance();

	if (db == "all")
	{
		for (vector<string>::iterator dbi = mDBs.begin(); dbi != mDBs.end(); ++dbi)
		{
			try
			{
				WSSearchNS::ns__DatabankInfo inf;
				GetDatabankInfo(*dbi, tbl[*dbi], inf);
				info.push_back(inf);
			}
			catch (...)
			{
				cerr << endl << "Skipping db " << *dbi << endl;
			}
		}
	}
	else if (find(mDBs.begin(), mDBs.end(), db) != mDBs.end())
	{
		WSSearchNS::ns__DatabankInfo inf;
		GetDatabankInfo(db, tbl[db], inf);
		info.push_back(inf);
	}
	else
		THROW(("Unknown databank '%s'", db.c_str()));

	return SOAP_OK;
}

void WSSearch::GetDatabankInfo(
	string							inID,
	CDatabankPtr					inMrsDb,
	WSSearchNS::ns__DatabankInfo&	outInfo)
{
	outInfo.id =		inID;
	outInfo.name =		inMrsDb->GetName();
	outInfo.script =	inMrsDb->GetScriptName();
	outInfo.url =		inMrsDb->GetInfoURL();
	outInfo.blastable = inMrsDb->GetBlastDbCount() > 0;
	
	outInfo.files.clear();

#warning("fix me")
// only one file left... (I used to parse the name tokenizing by + and | characters)
	{
		WSSearchNS::ns__FileInfo fi;
		
		fi.id =				inID;
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

int WSSearch::GetEntry(
	std::string					db,
	std::string					id,
	enum WSSearchNS::ns__Format	format,
	std::string&				entry)
{
	Log() << db << ':' << id;
	
	CDatabankPtr mrsDb = GetDatabank(db);
	
	uint32 docNr = mrsDb->GetDocumentNr(id);
	
	switch (format)
	{
		case WSSearchNS::plain:
			Log() << ':' << "plain";
			entry = mrsDb->GetDocument(docNr);
			break;
		
		case WSSearchNS::title:
			Log() << ':' << "title";
			if (not mrsDb->GetMetaData(docNr, "title", entry))
			{
				entry = mrsDb->GetDocument(docNr);
				entry = WFormatTable::Format(mrsDb->GetScriptName(), "title", entry, db, id);
			}
			break;
		
		case WSSearchNS::fasta:
		{
			Log() << ':' << "fasta";

			string sequence;
			
			if (mrsDb->GetBlastDbCount() > 0)
			{
				if (not mrsDb->GetSequence(docNr, 0, sequence))
					THROW(("Sequence for entry %s not found in databank %s", id.c_str(), db.c_str()));
			}
			else
			{
				if (not mrsDb->GetDocument(docNr, sequence))
					THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
					
				sequence = WFormatTable::Format(mrsDb->GetScriptName(), "sequence", sequence, db, id);
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
		
		case WSSearchNS::html:
			Log() << ':' << "html";
			entry = mrsDb->GetDocument(docNr, entry);
			entry = WFormatTable::Format(mrsDb->GetScriptName(), "html", entry, db, id);
			break;
		
		default:
			THROW(("Unsupported format in GetEntry"));
	}
	
	return SOAP_OK;
}

int WSSearch::GetIndices(
	std::string						db,
	std::vector<struct WSSearchNS::ns__Index>&	indices)
{
	Log() << db;
	
	CDatabankPtr mrsDb = GetDatabank(db);
	
	if (mrsDb->GetIndexCount() == 0)
		THROW(("Databank %s has no indices", db.c_str()));
	
	string script = mrsDb->GetScriptName();
	
	for (uint32 nix = 0; nix < mrsDb->GetIndexCount(); ++nix)
	{
		WSSearchNS::ns__Index ix;
		
		string type;
		uint32 count;
		mrsDb->GetIndexInfo(nix, ix.id, type, count);
		
		ix.description = WFormatTable::IndexName(script, ix.id);
		ix.count = count;
		
		if (type == "text" or type == "wtxt")
			ix.type = WSSearchNS::FullText;
		else if (type == "valu")
			ix.type = WSSearchNS::Unique;
		else if (type == "date")
			ix.type = WSSearchNS::Date;
		else if (type == "nmbr")
			ix.type = WSSearchNS::Number;
		
		indices.push_back(ix);
	}

	return SOAP_OK;
}

int WSSearch::Find(
	std::string					db,
	std::vector<std::string>	queryterms,
	enum WSSearchNS::ns__Algorithm			algorithm,
	bool						alltermsrequired,
	std::string					booleanfilter,
	int							resultoffset,
	int							maxresultcount,
	struct WSSearchNS::ns__FindResponse&	response)
{
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(Log(), ","));
	
	if (booleanfilter.length())
		Log() << ':' << '[' << booleanfilter << ']';

	CDatabankPtr mrsDb = GetDatabank(db);
	
	if (mrsDb.get() == nil)
		THROW(("Databank not found: %s", db.c_str()));
	
	auto_ptr<CDocIterator> r;
	uint32 count;
	PerformSearch(*mrsDb, queryterms, algorithm,
		alltermsrequired, booleanfilter, resultoffset + maxresultcount, r, count);
	
	if (count > 0)
	{
		uint32 docNr;
		while (resultoffset-- > 0 and r->Next(docNr, false))
			;
		
		while (maxresultcount-- > 0)
		{
			WSSearchNS::ns__Hit h;

			h.id = mrsDb->GetDocumentID(docNr);
			h.score = r->Score();
			h.db = db;
			h.title = GetTitle(db, h.id);
			
			response.hits.push_back(h);
		}

		response.count = r->Count(true);
	}
	else
		response.count = 0;

	return SOAP_OK;
}

int WSSearch::FindSimilar(
	std::string					db,
	std::string					id,
	enum WSSearchNS::ns__Algorithm			algorithm,
	int							resultoffset,
	int							maxresultcount,
	struct WSSearchNS::ns__FindResponse&	response)
{
	CDatabankPtr mrsDb = GetDatabank(db);
	
	string data;
	if (not mrsDb->Get(id, data))
		THROW(("Entry '%s' not found in '%d'", id.c_str(), db.c_str()));

	string entry = WFormatTable::Format(mrsDb->GetScriptName(), "indexed", entry, db, id);
	
	auto_ptr<MRankedQuery> q(mrsDb->RankedQuery("__ALL_TEXT__"));

	switch (algorithm)
	{
		case WSSearchNS::Vector:
			q->SetAlgorithm("vector");
			break;
		
		case WSSearchNS::Dice:
			q->SetAlgorithm("dice");
			break;
		
		case WSSearchNS::Jaccard:
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
			WSSearchNS::ns__Hit h;

			h.id = id;
			h.score = r->Score();
			h.db = db;
			h.title = GetTitle(db, h.id);
			
			response.hits.push_back(h);
		}

		response.count = r->Count(true);
	}

	return SOAP_OK;
}

struct SortOnFirstHitScore
{
	bool operator()(const WSSearchNS::ns__FindAllResult& a, const WSSearchNS::ns__FindAllResult& b) const
		{ return a.hits.front().score > b.hits.front().score; }
};

int WSSearch::FindAll(
	std::vector<std::string>	queryterms,
	enum WSSearchNS::ns__Algorithm			algorithm,
	bool						alltermsrequired,
	std::string					booleanfilter,
	std::vector<struct WSSearchNS::ns__FindAllResult>&
								response)
{
	copy(queryterms.begin(), queryterms.end(), ostream_iterator<string>(Log(), ","));
	
	if (booleanfilter.length())
		Log() << ':' << '[' << booleanfilter << ']';

	for (WSDatabankTable::iterator dbi = mDBs.begin(); dbi != mDBs.end(); ++dbi)
	{
		if (mDBs.Ignore(dbi->first))
			continue;

		auto_ptr<MQueryResults> r;
		try
		{
			r.reset(PerformSearch(
				*dbi->second.mDB, queryterms, algorithm, alltermsrequired, booleanfilter).release());
		}
		catch (...)
		{
			continue;
		}

		if (r.get() != NULL)
		{
			WSSearchNS::ns__FindAllResult fa;

			fa.db = dbi->first;
			
			const char* id;
			int n = 5;
			
			while (n-- > 0 and (id = r->Next()) != NULL)
			{
				WSSearchNS::ns__Hit h;
	
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

	return SOAP_OK;
}

int WSSearch::FindAllSimilar(
	std::string					db,
	std::string					id,
	enum WSSearchNS::ns__Algorithm			algorithm,
	std::vector<struct WSSearchNS::ns__FindAllResult>&
								response)
{
	Log() << db << ':' << id;
	
	CDatabankPtr mrsDb = GetDatabank(db);
	
	string data;
	if (not mrsDb->Get(id, data))
		THROW(("Entry '%s' not found in '%d'", id.c_str(), db.c_str()));

	string entry = WFormatTable::Format(mrsDb->GetScriptName(), "indexed", data, db, id);

	for (WSDatabankTable::iterator dbi = mDBs.begin(); dbi != mDBs.end(); ++dbi)
	{
		if (mDBs.Ignore(dbi->first))
			continue;

		try
		{
			auto_ptr<MRankedQuery> q(dbi->second.mDB->RankedQuery("__ALL_TEXT__"));
		
			switch (algorithm)
			{
				case WSSearchNS::Vector:
					q->SetAlgorithm("vector");
					break;
				
				case WSSearchNS::Dice:
					q->SetAlgorithm("dice");
					break;
				
				case WSSearchNS::Jaccard:
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
				WSSearchNS::ns__FindAllResult fa;

				fa.db = dbi->first;
				
				const char* id;
				int n = 5;
				
				while (n-- > 0 and (id = r->Next()) != NULL)
				{
					WSSearchNS::ns__Hit h;
		
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

	return SOAP_OK;
}

int WSSearch::Count(
	std::string					db,
	std::string					booleanquery,
	unsigned long&				response)
{
	Log() << db << ':' << booleanquery;
	
	response = 0;
	
	CDatabankPtr mrsDb = GetDatabank(db);

	auto_ptr<MQueryResults> r(mrsDb->Find(booleanquery));
	if (r.get() != nil)
		response = r->Count(true);

	return SOAP_OK;
}

struct CoTerm
{
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

int WSSearch::Cooccurrence(
	std::string					db,
	std::vector<std::string>	ids,
	float						idf_cutoff,
	int							resultoffset,
	int							maxresultcount,
	std::vector<std::string>&	response)
{
	Log() << db << ':';
	copy(ids.begin(), ids.end(), ostream_iterator<string>(Log(), ","));
	
	if (idf_cutoff < 0)
		idf_cutoff = 0;
	
	CDatabankPtr mrsDb = GetDatabank(db);

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

		string entry = WFormatTable::Format(mrsDb->GetScriptName(), "indexed", data, db, *id);

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
	
	vector<CoTerm> coterms;
	coterms.reserve(r.size());

	auto_ptr<MIndex> index(mrsDb->Index("__ALL_TEXT__"));
	
	for (CoTermSet::iterator t = r.begin(); t != r.end(); ++t)
	{
		if (t->count > 1)
		{
			try
			{
				const_cast<CoTerm&>(*t).idf = index->GetIDF(t->term);
				
				if (t->idf > idf_cutoff)
					coterms.push_back(*t);
			}
			catch (...) {}
		}
	}

	// do we have enough coterms?
	if (static_cast<int32>(coterms.size()) > maxresultcount - resultoffset)
	{
		sort(coterms.begin(), coterms.end(), CoTermScoreGreater());
		
		vector<CoTerm>::iterator t = coterms.begin();
		int32 n = 0;
		
		while (n < resultoffset and t != coterms.end())
		{
			++t;
			++n;
		}
		
		while (n < resultoffset + maxresultcount and t != coterms.end())
		{
			response.push_back(t->term);
			++t;
			++n;
		}
	}

	return SOAP_OK;
}

int WSSearch::SpellCheck(
	std::string					db,
	std::string					queryterm,
	std::vector<std::string>&	suggestions)
{
	Log() << db << ':' << queryterm;

	CDatabankPtr mrsDb = GetDatabank(db);
	
	auto_ptr<MStringIterator> s(mrsDb->SuggestCorrection(queryterm));
	if (s.get() != NULL)
	{
		const char* sw;
		while ((sw = s->Next()) != NULL)
			suggestions.push_back(sw);
	}

	return SOAP_OK;
}

// --------------------------------------------------------------------

CDatabankPtr WSSearch::GetDatabank(
	const std::string&			id)
{
	if (find(mDBs.begin(), mDBs.end(), db) == mDBs.end())
		THROW(("Unknown databank '%s'", db.c_str()));
	
	WSDatabankTable& tbl = WSDatabankTable::Instance();
	return tbl[db];
}

string WSSearch::GetTitle(
	const string&				db,
	const string&				id)
{
	CDatabankPtr mrsDb = GetDatabank(db);

	string result;
	
	if (not mrsDb->GetMetaData(id, "title", result) and
		mrsDb->Get(id, result))
	{
		result = WFormatTable::Format(mrsDb->GetScriptName(), "title", result, db, id);
	}
	
	return result;
}

int WSSearch::Serve(
	struct soap*			soap)
{
	return WSSearchNS::WSSearchNS_serve_request(soap);
}

void WSSearch::PerformSearch(
	CDatabank&					db,
	vector<string>				queryterms,
	enum WSSearchNS::ns__Algorithm
								algorithm,
	bool						alltermsrequired,
	string						booleanfilter,
	uint32						maxresultcount,
	auto_ptr<CDocIterator>&		outResult,
	uint32&						outCount)
{
	vector<string>::iterator qt = queryterms.begin();

	while (qt != queryterms.end())
	{
		string::size_type h;
		
		if (qt->find('*') != string::npos or qt->find('?') != string::npos)
		{
			booleanfilter += " ";
			booleanfilter += *qt;
			
			qt = queryterms.erase(qt);
		}
		else if ((h = qt->find('-')) != string::npos and qt->find('-', h + 1) == string::npos)
		{
			string t1 = qt->substr(0, h);
			string t2 = qt->substr(h + 1, string::npos);
			
//			booleanfilter += " \'" + t1 + ' ' + t2 + '\'';
			
			qt = queryterms.erase(qt);

			qt = queryterms.insert(qt, t2);
			qt = queryterms.insert(qt, t1);
		}
		else
			++qt;
	}

	if (booleanfilter.length())
	{
		CParsedQueryObject p(*mrsDb, booleanfilter, false);
		outResult.reset(p.Perform());
		outCount = outResult->Count();
	}
	
	if (queryterms.size() > 0)
	{
		// first determine the set of value indices, needed as a special case in ranked searches
		set<string> valueIndices;

		for (uint32 nix = 0; nix < mrsDb->GetIndexCount(); ++nix)
		{
			string id, type;
			uint32 count;
			
			mrsDb->GetIndexInfo(nix, id, type, count);
			if (type == "valu")
				valueIndices.push_back(id);
		}

		CRankedQuery query;
		string algo;
	
		switch (algorithm)
		{
			case WSSearchNS::Vector:
				algo = "vector";
				break;
			
			case WSSearchNS::Dice:
				algo = "dice";
				break;
			
			case WSSearchNS::Jaccard:
				algo = "jaccard";
				break;
			
			default:
				THROW(("Unsupported search algorithm"));
				break;
		}
		
		q->SetAllTermsRequired(alltermsrequired);
		
		for (vector<string>::iterator qw = queryterms.begin(); qw != queryterms.end(); ++qw)
			query.AddTerm(*qw, 1);
		
		query.PerformSearch(*mrsDb, "__ALL_TEXT__",
			algo, outResult, maxresultcount, alltermsrequired, result, count);
	}
}
