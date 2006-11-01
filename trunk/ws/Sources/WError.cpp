/*	$Id: CDatabank.cpp 28 2006-04-23 12:31:30Z maarten $
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#include <iostream>

#include "WError.h"

ws_exception::ws_exception(const char* inMessage, ...)
{
	using namespace std;
	
	va_list vl;
	va_start(vl, inMessage);
	vsnprintf(msg, sizeof(msg), inMessage, vl);
	va_end(vl);
#ifndef NDEBUG
	cerr << msg << endl;
#endif
}
