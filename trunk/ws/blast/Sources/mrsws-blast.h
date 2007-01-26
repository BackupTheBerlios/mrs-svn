// MRS web services interface

//gsoap ns service name: mrsws-blast
//gsoap ns service style: document
//gsoap ns service encoding: literal
//gsoap ns service location: http://localhost/mrs/cgi-bin/mrsws-blast
//gsoap ns schema namespace: http://mrs.cmbi.ru.nl/mrsws-blast
//gsoap ns service method-action: info ""

#include <vector>
template <class T> class std::vector;

typedef std::string xsd__string;

struct ns__Hsp
{
	unsigned long				score;
	double						bit_score;
	double						expect;
	unsigned long				query_start;
	unsigned long				subject_start;
	xsd__string					query_alignment;
	xsd__string					subject_alignment;
};

struct ns__Hit
{
	xsd__string					id;
	std::vector<struct ns__Hsp>	hsps;
};

int ns__BlastSync(
	xsd__string					db,
	xsd__string					query,
	xsd__string					program = "blastp",
	xsd__string					matrix = "BLOSUM62",
	unsigned long				word_size = 3,
	double						expect = 10,
	bool						low_complexity_filter = true,
	bool						gapped = true,
	unsigned long				gap_open = 11,
	unsigned long				gap_extend = 1,
	std::vector<struct ns__Hit>&response);

int ns__BlastAsync(
	xsd__string					db,
	xsd__string					query,
	xsd__string					program = "blastp",
	xsd__string					matrix = "BLOSUM62",
	unsigned long				word_size = 3,
	double						expect = 10,
	bool						low_complexity_filter = true,
	bool						gapped = true,
	unsigned long				gap_open = 11,
	unsigned long				gap_extend = 1,
	xsd__string&				response);

enum ns__JobStatus
{
	unknown,
	queued,
	running,
	error,
	finished
};

int ns__BlastJobStatus(
	xsd__string					job_id,
	enum ns__JobStatus&			response);

int ns__BlastJobResult(
	xsd__string					job_id,
	std::vector<struct ns__Hit>&response);

