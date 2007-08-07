/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday February 07 2007 09:05:53
*/

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cerrno>
#include <pwd.h>
#include <fcntl.h>
#include <boost/thread.hpp>

#include "WUtils.h"

using namespace std;

extern double system_time();

namespace {
	
static boost::mutex sStdOutLock;
	
}


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
	boost::mutex::scoped_lock lock(sStdOutLock);
	
	cout.setf(ios::fixed);
	cout << mMsg << ' ' << setprecision(3) << system_time() - mStart << endl;
}

WLogger& WLogger::operator<<(
	const string&	inParam)
{
	mMsg += inParam;
//	mMsg += ' ';

	return *this;
}

WLogger& WLogger::operator<<(
	char			inChar)
{
	mMsg += inChar;

	return *this;
}

// --------------------------------------------------------------------
//
//	Daemonize
// 

void Daemonize(
	const string&		inUser,
	const string&		inLogFile,
	const char*			inPIDFile,
	ofstream&			outLogFileStream)
{
	int pid = fork();
	
	if (pid == -1)
	{
		cerr << "Fork failed" << endl;
		exit(1);
	}
	
	if (pid != 0)
	{
		cout << "Started daemon with process id: " << pid << endl;
		
		ofstream pidFile(inPIDFile);
		pidFile << pid << endl;
		pidFile.close();
		
		_exit(0);
	}

	// open the log file
	int fd = open(inLogFile.c_str(), O_CREAT|O_APPEND|O_RDWR, 0644);
	if (fd < 0)
		cerr << "Opening log file " << inLogFile << " failed" << endl;
	
	if (chdir("/") != 0)
	{
		cerr << "Cannot chdir to /: " << strerror(errno) << endl;
		exit(1);
	}

	if (setsid() < 0)
	{
		cerr << "Failed to create process group: " << strerror(errno) << endl;
		exit(1);
	}

	if (inUser.length() > 0)
	{
		struct passwd* pw = getpwnam(inUser.c_str());
		if (pw == NULL or setuid(pw->pw_uid) < 0)
		{
			cerr << "Failed to set uid to " << inUser << ": " << strerror(errno) << endl;
			exit(1);
		}
	}

	// redirect stdout and stderr to the log file
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);

	// close stdin
	close(STDIN_FILENO);
	open("/dev/null", O_RDONLY);
}
