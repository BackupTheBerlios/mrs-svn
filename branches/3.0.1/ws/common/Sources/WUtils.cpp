/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday February 07 2007 09:05:53
*/

#include <sstream>
#include <iostream>
#include <iomanip>

#include "WUtils.h"

using namespace std;

extern double system_time();

// --------------------------------------------------------------------
//
//	Utility routines
// 

WLogger::WLogger(
	unsigned long	inIPAddress,
	const char*		inFunction)
{
	mStart = system_time();

	time_t now;
	time(&now);
	
	struct tm tm;
	localtime_r(&now, &tm);
	
	char s[1024];
	strftime(s, sizeof(s), "[%d/%b/%Y:%H:%M:%S]", &tm);

	stringstream ss;
	
	ss << ((inIPAddress >> 24) & 0xFF) << '.'
	   << ((inIPAddress >> 16) & 0xFF) << '.'
	   << ((inIPAddress >>  8) & 0xFF) << '.'
	   << ( inIPAddress        & 0xFF) << ' '
	   << s << ' '
	   << inFunction << ' ';
	
	mMsg = ss.str();
}

WLogger::~WLogger()
{
	cout.setf(ios::fixed);
	cout << mMsg << setprecision(3) << system_time() - mStart << endl;
}

WLogger& WLogger::operator<<(
	const string&	inParam)
{
	mMsg += inParam;
	mMsg += ' ';

	return *this;
}

WLogger& WLogger::operator<<(
	char			inChar)
{
	mMsg += inChar;

	return *this;
}

