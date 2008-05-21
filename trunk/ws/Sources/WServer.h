#ifndef WSERVER_H
#define WSERVER_H

#include <boost/thread.hpp>
#include <sstream>

#include "stdsoap2.h"

class WServer
{
  public:
						WServer(
							const std::string&			inAddress,
							uint16						inPortNr,
							SOAP_NMAC struct Namespace	inNameSpaces[]);

	virtual				~WServer();

	int					GetFD() const					{ return mMasterSocket; }
	
	void				Accept() throw ();
	
	virtual void		Stop();

  protected:

	virtual int			Serve(
							struct soap*				inSoap) = 0;

	virtual void		Log(
							const char*					inMessage,
							...) throw ();
	
	virtual void		Log(
							const std::string&			inMessage);

	virtual std::ostream&
						Log()							{ return *mLog; }
	
	virtual void		LogFault(
							std::ostream&				inLogger) throw ();

	struct soap			mSoap;
	
  private:
	int					mMasterSocket;
	std::ostream*		mLog;
	std::string			mAction;
};

template<class S>
class WServerT : public WServer
{
  public:
						WServerT(
							const std::string&			inAddress,
							uint16						inPortNr,
							SOAP_NMAC struct Namespace	inNameSpaces[])
							: WServer(inAddress, inPortNr, inNameSpaces)
						{
						}
		
		static S*		GetServer(
							struct soap*				inSoap)
						{
							WServer* server = reinterpret_cast<WServer*>(inSoap->user);
							if (server == nil)
								THROW(("Run time error"));
							S* result = dynamic_cast<S*>(server);
							ThrowIfNil(result);
							return result;
						}
};

#endif
