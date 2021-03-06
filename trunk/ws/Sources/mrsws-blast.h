// MRS web services interface

namespace WSBlastNS {

//gsoap ns service name: mrsws-blast
//gsoap ns service style: document
//gsoap ns service encoding: literal
//gsoap ns service location: http://mrs.cmbi.ru.nl/mrsws-blast
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
	unsigned long				identity;
	unsigned long				positive;
	unsigned long				gaps;
	unsigned long				subject_length;	// this one is needed in the interface
	xsd__string					query_alignment;
	xsd__string					subject_alignment;
	xsd__string					midline;
};

struct ns__Hit
{
	xsd__string					id;
	xsd__string					title;
	std::vector<struct ns__Hsp>	hsps;
};

struct ns__BlastResult {
	unsigned long				db_count;
	unsigned long				db_length;
	unsigned long				eff_space;
	double						kappa;
	double						lambda;
	double						entropy;
	std::vector<struct ns__Hit>	hits;
};

int ns__BlastSync(
	xsd__string					db,
	xsd__string					mrsBooleanQuery = "",
	xsd__string					query,
	xsd__string					program = "blastp",
	xsd__string					matrix = "BLOSUM62",
	unsigned long				word_size = 3,
	double						expect = 10,
	bool						low_complexity_filter = true,
	bool						gapped = true,
	unsigned long				gap_open = 11,
	unsigned long				gap_extend = 1,
	unsigned long				report_limit = 250,
	struct ns__BlastResult&		response);

int ns__BlastAsync(
	xsd__string					db,
	xsd__string					mrsBooleanQuery = "",
	xsd__string					query,
	xsd__string					program = "blastp",
	xsd__string					matrix = "BLOSUM62",
	unsigned long				word_size = 3,
	double						expect = 10,
	bool						low_complexity_filter = true,
	bool						gapped = true,
	unsigned long				gap_open = 11,
	unsigned long				gap_extend = 1,
	unsigned long				report_limit = 250,
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
	struct ns__BlastResult&		response);

int ns__BlastJobError(
	xsd__string					job_id,
	xsd__string&				response);

}
