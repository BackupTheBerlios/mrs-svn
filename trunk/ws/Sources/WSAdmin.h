#ifndef WSADMIN_H
#define WSADMIN_H

#include <map>

#include "WServer.h"
#include "WSAdminNSH.h"

class WSAdmin : public WServerT<WSAdmin>
{
	friend class WServerT<WSAdmin>;
	
	typedef WSAdminNS::ns__ProviderStatus		ProviderStatus;
	typedef WSAdminNS::ns__ProviderStatusInfo	ProviderStatusInfo;
	typedef WSAdminNS::ns__ParserScriptFormat	ParserScriptFormat;
	
  public:
					WSAdmin(
						const std::string&		inAddress,
						uint16					inPortNr);
	
	int 			GetProviderStatusInfo(
						std::string&			provider,
						std::vector<ProviderStatusInfo>&
												response);

	int 			SetProviderStatusInfo(
						std::string				provider,
						ProviderStatus			status,
						float					progress,
						std::string				message,
						int&					response);
	
	int 			GetProviderMirrorLog(
						std::string				provider,
						unsigned long			age,
						std::string&			response);
	
	int 			GetProviderMakeLog(
						std::string				provider,
						unsigned long			age,
						std::string&			response);
	
	int 			GetParserScript(
						std::string				script,
						ParserScriptFormat		format,
						std::string&			response);

  protected:

	virtual int		Serve(
						struct soap*			inSoap);

	typedef std::vector<WSAdminNS::ns__ProviderStatusInfo>	StatusTable;
	StatusTable		mStatus;
};

#endif
