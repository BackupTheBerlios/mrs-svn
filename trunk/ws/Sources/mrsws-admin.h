// MRS web services interface

namespace WSAdminNS {

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

enum ns__ProviderStatus
{
	ready,
	mirroring,
	indexing,
	error
};

struct ns__ProviderStatusInfo
{
	xsd__string					name;
	enum ns__ProviderStatus		status;
	float						progress;
	xsd__string					message;
	std::vector<xsd__string>	databanks;
};

int ns__GetProviderStatusInfo(
	xsd__string					db,
	std::vector<struct ns__ProviderStatusInfo>&
								response);

int ns__GetProviderMirrorLog(
	xsd__string					db,
	unsigned long				age = 0,
	xsd__string&				response);

int ns__GetProviderMakeLog(
	xsd__string					db,
	unsigned long				age = 0,
	xsd__string&				response);

enum ns__ParserScriptFormat
{
	plain,
	html
};

int ns__GetParserScript(
	xsd__string					script,
	enum ns__ParserScriptFormat	format = html,
	xsd__string&				response);

// to store information

int ns__SetProviderStatusInfo(
	xsd__string					db,
	enum ns__ProviderStatus		status,
	float						progress = 1.0,
	xsd__string					message = "",
	int&						response);

}
