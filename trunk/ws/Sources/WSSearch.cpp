#include "HLib.h"

#include <set>
#include <boost/regex.hpp>

#include "HError.h"
#include "MRSInterface.h"

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
	DBInfoVector&	inDbs)
	: WServerT<WSSearch>(inAddress, inPortNr, WSSearchNS_namespaces)
	, mDBs(inDbs)
{
}

int WSSearch::GetDatabankInfo(
	string											db,
	vector<struct WSSearchNS::ns__DatabankInfo>&	info)
{
	Log() << db;

	if (db == "all")
	{
		for (WSDatabankTable::iterator dbi = mDBs.begin(); dbi != mDBs.end(); ++dbi)
		{
			try
			{
				WSSearchNS::ns__DatabankInfo inf;
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
		WSSearchNS::ns__DatabankInfo inf;
		GetDatabankInfo(mDBs[db], inf);
		info.push_back(inf);
	}

	return SOAP_OK;
}

void WSSearch::GetDatabankInfo(
	MDatabankPtr					inMrsDb,
	WSSearchNS::ns__DatabankInfo&	outInfo)
{
	outInfo.id =		inMrsDb->GetCode();
	outInfo.name =		inMrsDb->GetName();
	outInfo.script =	inMrsDb->GetScriptName();
	outInfo.url =		inMrsDb->GetInfoURL();
	outInfo.blastable = inMrsDb->ContainsBlastIndex();
	
	outInfo.files.clear();

// only one file left... (I used to parse the name tokenizing by + and | characters)
	{
		WSSearchNS::ns__FileInfo fi;
		
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

int WSSearch::GetEntry(
	std::string					db,
	std::string					id,
	enum WSSearchNS::ns__Format	format,
	std::string&				entry)
{
	Log() << db << ':' << id;
	
	MDatabankPtr mrsDb = mDBs[db];
	
	switch (format)
	{
		case WSSearchNS::plain:
			Log() << ':' << "plain";
			if (not mrsDb->Get(id, entry))
				THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
			break;
		
		case WSSearchNS::title:
			Log() << ':' << "title";
			if (not mrsDb->GetMetaData(id, "title", entry) and
				mrsDb->Get(id, entry))
			{
				entry = WFormatTable::Format(mrsDb->GetScriptName(), "title", entry, db, id);
			}
			break;
		
		case WSSearchNS::fasta:
		{
			Log() << ':' << "fasta";

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
		{
			Log() << ':' << "html";

			if (not mrsDb->Get(id, entry))
				THROW(("Entry %s not found in databank %s", id.c_str(), db.c_str()));
				
			entry = WFormatTable::Format(mrsDb->GetScriptName(), "html", entry, db, id);
			break;
		}
		
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
	
	MDatabankPtr mrsDb = mDBs[db];
	
	auto_ptr<MIndices> mrsIndices(mrsDb->Indices());
	
	if (not mrsIndices.get())
		THROW(("Databank %s has no indices", db.c_str()));
	
	string script = mrsDb->GetScriptName();
	
	for (auto_ptr<MIndex> index(mrsIndices->Next()); index.get() != NULL; index.reset(mrsIndices->Next()))
	{
		WSSearchNS::ns__Index ix;
		
		ix.id = index->Code();
		ix.description = WFormatTable::IndexName(script, ix.id);
		ix.count = index->Count();
		
		string t = index->Type();
		if (t == "text" or t == "wtxt")
			ix.type = WSSearchNS::FullText;
		else if (t == "valu")
			ix.type = WSSearchNS::Unique;
		else if (t == "date")
			ix.type = WSSearchNS::Date;
		else if (t == "nmbr")
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

	MDatabankPtr mrsDb = mDBs[db];
	
	if (mrsDb.get() == nil)
		THROW(("Databank not found: %s", db.c_str()));
	
	auto_ptr<MQueryResults> r =
		PerformSearch(*mrsDb, queryterms, algorithm, alltermsrequired, booleanfilter);
	
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
	MDatabankPtr mrsDb = mDBs[db];
	
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
	
	MDatabankPtr mrsDb = mDBs[db];
	
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
	
	MDatabankPtr mrsDb = mDBs[db];
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
	
	MDatabankPtr mrsDb = mDBs[db];

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

	MDatabankPtr mrsDb = mDBs[db];
	
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

string WSSearch::GetTitle(
	const string&				db,
	const string&				id)
{
	MDatabankPtr mrsDb(mDBs[db]);

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

auto_ptr<MQueryResults>
WSSearch::PerformSearch(
	MDatabank&					db,
	vector<string>				queryterms,
	enum WSSearchNS::ns__Algorithm
								algorithm,
	bool						alltermsrequired,
	string						booleanfilter)
{
	auto_ptr<MQueryResults> result;
	
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
	
	if (queryterms.size() > 0)
	{
		// first determine the set of value indices, needed as a special case in ranked searches
		set<string> valueIndices;

		auto_ptr<MIndices> indices(db.Indices());
		MIndex* index;
		while ((index = indices->Next()) != NULL)
		{
			if (index->Type() == "valu")
				valueIndices.insert(index->Code());
			delete index;
		}

		auto_ptr<MRankedQuery> q(db.RankedQuery("__ALL_TEXT__"));
	
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
			
		q->SetAllTermsRequired(alltermsrequired);
		
		for (vector<string>::iterator qw = queryterms.begin(); qw != queryterms.end(); ++qw)
			q->AddTerm(*qw, 1);
		
		auto_ptr<MBooleanQuery> m;
		if (booleanfilter.length())
			m.reset(db.BooleanQuery(booleanfilter));

		result.reset(q->Perform(m.get()));
	}
	else if (booleanfilter.length() > 0)
		result.reset(db.Find(booleanfilter, false));
	
	return result;
}
