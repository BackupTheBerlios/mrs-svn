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
#include <signal.h>
#include <boost/thread.hpp>
#include <signal.h>
#include <sys/time.h>

#include "stdsoap2.h"

#include "WUtils.h"

using namespace std;

namespace {
	
static boost::mutex sStdOutLock;

double system_time()
{
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	
	return tv.tv_sec + tv.tv_usec / 1e6;
}
	
}

// --------------------------------------------------------------------
//
//	Utility routines
// 

WLogger::WLogger(
	struct soap*	inSoap)
{
	mStart = system_time();

	time_t now;
	time(&now);
	
	struct tm tm;
	localtime_r(&now, &tm);
	
	char s[1024];
	strftime(s, sizeof(s), "[%d/%b/%Y:%H:%M:%S]", &tm);
	
	long ip = inSoap->ip;

	(*this) << ((inIPAddress >> 24) & 0xFF) << '.'
	   << ((inIPAddress >> 16) & 0xFF) << '.'
	   << ((inIPAddress >>  8) & 0xFF) << '.'
	   << ( inIPAddress        & 0xFF) << ' '
	   << s << ' '
	   << inSoap->action << ' ';
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

WLogger& WLogger::operator<<(
	float			inFloat)
{
	stringstream s;
	s << inFloat;
	mMsg += s.str();
	
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
//		ofstream pidFile(inPIDFile);
//		pidFile << pid << endl;
//		pidFile.close();
		
		_exit(0);
	}

	if (setsid() < 0)
	{
		cerr << "Failed to create process group: " << strerror(errno) << endl;
		exit(1);
	}

	// it is dubious if this is needed:
	signal(SIGHUP, SIG_IGN);

	// fork again, to avoid being able to attach to a terminal device
	pid = fork();

	if (pid == -1)
		cerr << "Fork failed" << endl;

	if (pid != 0)
		_exit(0);

	// write our pid to the pid file
	ofstream pidFile(inPIDFile);
	pidFile << getpid() << endl;
	pidFile.close();

	// open the log file
	int fd = open(inLogFile.c_str(), O_CREAT|O_APPEND|O_RDWR, 0644);
	if (fd < 0)
		cerr << "Opening log file " << inLogFile << " failed" << endl;
	
	if (chdir("/") != 0)
	{
		cerr << "Cannot chdir to /: " << strerror(errno) << endl;
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
