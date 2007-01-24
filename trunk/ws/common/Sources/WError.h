/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 15:07:37
*/

#ifndef WERROR_H
#define WERROR_H

#include <iostream>

using namespace std;

class ws_exception : public std::exception
{
  public:
						ws_exception(const char* msg, ...);
						
	virtual const char*	what() const throw()		{ return msg; }
	
  private:
	char				msg[1024];
};

inline void __report_throw(const char* ex, const char* file, int line)
{
#ifndef NDEBUG
	cerr << "Exception " << ex << " in File: " << file << " line: " << line << endl;
#endif
}

#define THROW(e)		do { __report_throw(#e, __FILE__, __LINE__); throw ws_exception e; } while (false)

#endif // WERROR_H
