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

struct ns__DatabankInfo
{
	xsd__string		id;
	xsd__string		uuid;
	xsd__string		name;
	xsd__string		version;
	xsd__string		parser;
	xsd__string		url;
	xsd__string		filter;
	xsd__string		last_update;
	bool			blastable;
	unsigned long	entries;		
};

int ns__GetDatabankInfo(xsd__string db = "all", std::vector<struct ns__DatabankInfo>& info);

enum ns__Format
{
	plain,
	title,
	html,
	fasta
};

int ns__GetEntry(xsd__string db, xsd__string id, enum ns__Format format = plain, xsd__string& entry);

struct ns__Hit
{
	xsd__string		db;
	xsd__string		id;
	xsd__string		title;
	float			score;
};

struct ns__FindResponse
{
	unsigned long					count;	// the total number of hits found
	std::vector<struct ns__Hit>		hits;
};

enum ns__Algorithm
{
	Vector,
	Dice,
	Jaccard
};

int ns__Find(xsd__string db, std::vector<xsd__string> queryterms, bool alltermsrequired = true,
	enum ns__Algorithm algorithm = Vector, int resultoffset = 0, int maxresultcount = 15,
	struct ns__FindResponse& response);
int ns__FindSimilar(xsd__string db, xsd__string id,
	enum ns__Algorithm algorithm = Vector, int resultoffset = 0, int maxresultcount = 15,
	struct ns__FindResponse& response);

