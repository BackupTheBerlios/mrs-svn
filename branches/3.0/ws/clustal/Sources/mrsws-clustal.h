// MRS web services interface

//gsoap ns service name: mrsws-clustal
//gsoap ns service style: document
//gsoap ns service encoding: literal
//gsoap ns service location: http://localhost/mrs/cgi-bin/mrsws-clustal
//gsoap ns schema namespace: http://mrs.cmbi.ru.nl/mrsws-clustal
//gsoap ns service method-action: info ""

#include <vector>
template <class T> class std::vector;

typedef std::string xsd__string;

struct ns__Sequence {
	xsd__string		id;
	xsd__string		sequence;
};

struct ns__ClustalWResponse {
	xsd__string							diagnostics_out;
	xsd__string							diagnostics_err;
	std::vector<struct ns__Sequence>	alignment;
};

int ns__ClustalW(
	std::vector<struct ns__Sequence>	input,
	struct ns__ClustalWResponse&		response);
