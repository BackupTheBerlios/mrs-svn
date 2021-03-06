/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday February 07 2007 09:05:27
*/

#ifndef WUTILS_H
#define WUTILS_H

#include <string>

class WLogger
{
  public:
				WLogger(unsigned long	inIPAddress,
						const char*		inFunction);
				~WLogger();

	WLogger&	operator<<(
					char				inChar);

	WLogger&	operator<<(
					const std::string&	inString);

  private:
	double		mStart;
	std::string	mMsg;
};

#endif // WUTILS_H
