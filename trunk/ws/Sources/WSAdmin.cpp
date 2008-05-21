#include "HLib.h"

#include "HError.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include "WSAdminNSH.h"
#include "WSAdmin.h"
#include "WConfig.h"
#include "WFormat.h"

#include "WSAdminNS.nsmap"

using namespace std;

namespace fs = boost::filesystem;

namespace {

string ReadFile(
	const fs::path&		inFile)
{
	if (not fs::exists(inFile))
		THROW(("File %s does not exist", inFile.string().c_str()));
		
	fs::ifstream file(inFile);
	if (not file.is_open())
		THROW(("Could not open file %s", inFile.string().c_str()));

	stringstream result;
	
	while (not file.eof())
	{
		string line;
		getline(file, line);
		result << line << endl;
	}
	
	return result.str();
}

}

// --------------------------------------------------------------------
//
//	SOAP calls
// 

namespace WSAdminNS {

SOAP_FMAC5 int SOAP_FMAC6
ns__GetProviderStatusInfo(
	struct soap*		soap,
	string				provider,
	vector<struct ns__ProviderStatusInfo>&
						response)
{
	return WSAdmin::GetServer(soap)->GetProviderStatusInfo(provider, response);
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetProviderMirrorLog(
	struct soap*		soap,
	string				provider,
	unsigned long		age,
	string&				response)
{
	return WSAdmin::GetServer(soap)->GetProviderMirrorLog(provider, age, response);
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetProviderMakeLog(
	struct soap*		soap,
	string				provider,
	unsigned long		age,
	string&				response)
{
	return WSAdmin::GetServer(soap)->GetProviderMakeLog(provider, age, response);
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetParserScript(
	struct soap*		soap,
	string				script,
	ns__ParserScriptFormat
						format,
	string&				response)
{
	return WSAdmin::GetServer(soap)->GetParserScript(script, format, response);
}

SOAP_FMAC5 int SOAP_FMAC6
ns__SetProviderStatusInfo(
	struct soap*		soap,
	string				provider,
	enum ns__ProviderStatus
						status,
	float				progress,
	string				message,
	int&				response)
{
	return WSAdmin::GetServer(soap)->SetProviderStatusInfo(provider, status, progress, message, response);
}

}

// --------------------------------------------------------------------
//

WSAdmin::WSAdmin(
	const string&		inAddress,
	uint16				inPort)
	: WServerT<WSAdmin>(inAddress, inPort, WSAdminNS_namespaces)
{
}

int WSAdmin::GetProviderStatusInfo(
	string&						provider,
	vector<ProviderStatusInfo>&	response)
{
	Log() << provider;
		
	if (provider == "all")
		response = mStatus;
	else
	{
		response.clear();

		for (StatusTable::iterator di = mStatus.begin(); di != mStatus.end(); ++di)
		{
			if (di->name == provider)
			{
				response.push_back(*di);
				break;
			}
		}

		if (response.size() == 0)
			THROW(("Provider not found: %s", provider.c_str()));
	}

	return SOAP_OK;
}

int WSAdmin::SetProviderStatusInfo(
	string						provider,
	ProviderStatus				status,
	float						progress,
	string						message,
	int&						response)
{
	THROW(("Unimplemented yet"));
}

int WSAdmin::GetProviderMirrorLog(
	string						provider,
	unsigned long				age,
	string&						response)
{
	THROW(("Unimplemented yet"));
}

int WSAdmin::GetProviderMakeLog(
	string						provider,
	unsigned long				age,
	string&						response)
{
	THROW(("Unimplemented yet"));
}

int WSAdmin::GetParserScript(
	string						script,
	ParserScriptFormat			format,
	string&						response)
{
	Log() << script;

	fs::path file = gParserDir / script;
	if (not fs::exists(file))
		file = gParserDir / (script + ".pm");
	
	response = ReadFile(file);
	
	if (format == WSAdminNS::html)
		response = WFormatTable::PPScript(response);

	return SOAP_OK;
}

int WSAdmin::Serve(
	struct soap*			inSoap)
{
	return WSAdminNS::WSAdminNS_serve_request(inSoap);
}
