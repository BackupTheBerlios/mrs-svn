// MRS web services interface

//gsoap ns service name: mrsws
//gsoap ns service style: document
//gsoap ns service encoding: literal
//gsoap ns service location: http://localhost/mrs/cgi-bin/mrsws
//gsoap ns schema namespace: http://mrs.cmbi.ru.nl/mrsws
//gsoap ns service method-action: info ""

#include <vector>
template <class T> class std::vector;

typedef std::string xsd__string;

struct ns__FileInfo
{
	xsd__string					id;
	xsd__string					uuid;
	xsd__string					version;
	xsd__string					path;
	xsd__string					modification_date;
	unsigned long				entries;
	unsigned long long			file_size;
	unsigned long long			raw_data_size;
};

struct ns__DatabankInfo
{
	xsd__string					id;
	xsd__string					name;
	xsd__string					parser;
	xsd__string					url;
	xsd__string					filter;
	bool						blastable;
	std::vector<struct ns__FileInfo>
								files;
};

int ns__GetDatabankInfo(
	xsd__string								db = "all",
	std::vector<struct ns__DatabankInfo>&	info);

enum ns__Format
{
	plain,
	title,
	html,
	fasta
};

int ns__GetEntry(
	xsd__string					db,
	xsd__string					id,
	enum ns__Format				format = plain,
	xsd__string&				entry);

enum ns__IndexType
{
	Unique,
	FullText,
	Number,
	Date
};

struct ns__Index
{
	xsd__string					id;
	xsd__string					description;
	enum ns__IndexType			type;
};

int ns__GetIndices(
	xsd__string						db,
	std::vector<struct ns__Index>&	indices);

struct ns__Hit
{
	xsd__string					db;
	xsd__string					id;
	xsd__string					title;
	float						score;
};

struct ns__FindResponse
{
	unsigned long				count;	// the total number of hits found
	std::vector<struct ns__Hit>	hits;	// the requested sub section of the results
};

enum ns__Algorithm
{
	Vector,
	Dice,
	Jaccard
};

int ns__Find(
	xsd__string					db,
	std::vector<xsd__string>	queryterms,
	enum ns__Algorithm			algorithm = Vector,
	bool						alltermsrequired = true,
	xsd__string					booleanfilter,
	int							resultoffset = 0,
	int							maxresultcount = 15,
	struct ns__FindResponse&	response);

struct ns__FindAllResult
{
	xsd__string					db;
	unsigned long				count;
};

int ns__FindAll(
	std::vector<xsd__string>	queryterms,
	enum ns__Algorithm			algorithm = Vector,
	bool						alltermsrequired = true,
	xsd__string					booleanfilter,
	std::vector<struct ns__FindAllResult>&
								response);
int ns__SpellCheck(
	xsd__string					db,
	xsd__string					queryterm,
	std::vector<xsd__string>&	suggestions);

//int ns__FindSimilar(xsd__string db, xsd__string id,
//	enum ns__Algorithm algorithm = Vector, int resultoffset = 0, int maxresultcount = 15,
//	struct ns__FindResponse& response);


