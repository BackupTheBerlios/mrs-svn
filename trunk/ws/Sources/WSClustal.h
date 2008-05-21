#ifndef WSCLUSTAL_H
#define WSCLUSTAL_H

#include "WServer.h"

class WSClustal : public WServer
{
  public:
					WSClustal(
						const char*		inAddress,
						uint16			inPortNr);
	
  protected:	

	virtual void	Init();

	virtual int		Serve(
						struct soap*	inSoap);
};

#endif
