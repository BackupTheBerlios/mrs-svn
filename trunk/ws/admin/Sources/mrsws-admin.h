// MRS web services interface

//gsoap ns service name: mrsws-admin
//gsoap ns service style: document
//gsoap ns service encoding: literal
//gsoap ns service location: http://localhost/mrs/cgi-bin/mrsws-admin
//gsoap ns schema namespace: http://mrs.cmbi.ru.nl/mrsws-admin
//gsoap ns service method-action: info ""

#include <vector>
template <class T> class std::vector;

typedef std::string xsd__string;

// information retrieval 

enum ns__DatabankStatus
{
	ready,
	mirroring,
	indexing,
	error
};

struct ns__DatabankStatusInfo
{
	xsd__string					name;
	enum ns__DatabankStatus		status;
	float						progress;
	xsd__string					message;
};

int ns__GetDatabankStatusInfo(
	xsd__string					db,
	std::vector<struct ns__DatabankStatusInfo>&
								response);

int ns__GetDatabankMirrorLog(
	xsd__string					db,
	unsigned long				age = 0,
	xsd__string&				response);

int ns__GetDatabankMakeLog(
	xsd__string					db,
	unsigned long				age = 0,
	xsd__string&				response);

int ns__GetParserScript(
	xsd__string					script,
	xsd__string&				response);

// to store information

int ns__SetDatabankStatusInfo(
	xsd__string					db,
	enum ns__DatabankStatus		status,
	float						progress = 1.0,
	xsd__string					message = "",
	int&						response);

