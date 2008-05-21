/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday February 07 2007 09:05:27
*/

#ifndef WUTILS_H
#define WUTILS_H

#include <string>
#include <sstream>

class WLogger : public std::stringstream
{
  public:
				WLogger(
					struct soap*		inSoap);

				~WLogger();

  private:
	double		mStart;
};

void Daemonize(
		const std::string&		inUser,
		const std::string&		inLogFile,
		const char*				inPIDFile,
		std::ofstream&			outLogFileStream);

#endif // WUTILS_H
