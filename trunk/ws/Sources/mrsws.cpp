/*	$Id: mrsws.cpp$
	Copyright Maarten L. Hekkelman
	Created: 10 maart 2008
*/

#include "mrsws.h"

#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <iomanip>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <cerrno>
#include <pwd.h>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>
#include <boost/foreach.hpp>

#include "WError.h"
#include "WConfig.h"
#include "WBuffer.h"
#include "WFormat.h"

#include "WSAdmin.h"
#include "WSBlast.h"
//#include "WSClustal.h"
#include "WSSearch.h"

SOAP_NMAC struct Namespace namespaces[0] = {};

#define nil NULL

using namespace std;
#define foreach BOOST_FOREACH

// globals for communication with the outside world

bool	gNeedReload;
bool	gQuit;

extern double system_time();

// --------------------------------------------------------------------
//	Debug code

#ifndef NDEBUG
bool gOKToThrow;

void ReportThrow(const char* inFunc, const char* inFile, int inLine)
{
	if (gOKToThrow)
		return;
	
	cerr << endl << "Exception in " << inFunc << ", " << inFile << ':' << inLine << endl;
}

#endif

// --------------------------------------------------------------------
//
//	Daemonize
// 

void Daemonize(
	const string&		inUser)
{
	int pid = fork();
	
	if (pid == -1)
	{
		cerr << "Fork failed" << endl;
		exit(1);
	}
	
	if (pid != 0)
		_exit(0);

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
	ofstream pidFile(MRS_PID_FILE);
	pidFile << getpid() << endl;
	pidFile.close();

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

	// close stdin
	close(STDIN_FILENO);
	open("/dev/null", O_RDONLY);
}

// --------------------------------------------------------------------
// 
//	OpenLogFile
//	

void OpenLogFile()
{
	// open the log file
	int fd = open(gLogFile.string().c_str(), O_CREAT|O_APPEND|O_RDWR, 0644);
	if (fd < 0)
	{
		cerr << "Opening log file " << gLogFile.string() << " failed" << endl;
		exit(1);
	}

	// redirect stdout and stderr to the log file
	dup2(fd, STDOUT_FILENO);
	dup2(fd, STDERR_FILENO);
}

// --------------------------------------------------------------------
// 
//	Main Loop
//	

void RunMainLoop()
{
	boost::ptr_vector<WServer> servers;
	
	string service, address;
	uint16 port;
	DBInfoVector dbs;

	fd_set fds;
	int nfds = 0;
	FD_ZERO(&fds);
	
	while (gConfig->NextServerConfig(service, address, port, dbs))
	{
		if (service == "admin")
			servers.push_back(new WSAdmin(address, port));
		else if (service == "search")
			servers.push_back(new WSSearch(address, port, dbs));
		else if (service == "blast")
			servers.push_back(new WSBlast(address, port, dbs));
//		else if (service == "clustal")
//			servers.push_back(new WSClustal(address.c_str(), port));
		else
		{
			cerr << "Unknown service: " << service << endl;
			continue;
		}
	}
	
	if (servers.size() == 0)
	{
		cerr << "No servers configured" << endl;
		exit(1);
	}
	
	gNeedReload = false;
	gQuit = false;

	// loop until told to quit or reload
	while (not (gQuit or gNeedReload))
	{
		// find out if any of our servers wants to write some log message

		foreach (WServer& server, servers)
		{
			int fd = server.GetFD();
			FD_SET(fd, &fds);
			if (fd >= nfds)
				nfds = fd + 1;
		}
		
		struct timeval timeout = { 5, 0 };
		
		int ready = select(nfds, &fds, nil, nil, &timeout);

		if (ready > 0)
		{
			foreach (WServer& server, servers)
			{
				if (FD_ISSET(server.GetFD(), &fds))
					server.Accept();
			}
		}
	}
	
	for_each(servers.begin(), servers.end(), boost::bind(&WServer::Stop, _1));
}

// --------------------------------------------------------------------
// 
//   main body
// 

void handler(int inSignal)
{
	int old_errno = errno;
	
	switch (inSignal)
	{
		case SIGINT:
			gQuit = true;
			break;
		
		case SIGHUP:
			gNeedReload = true;
			break;
		
		default:	// now what?
			break;
	}
	
	cout << "signal caught: " << inSignal << endl;
	
	errno = old_errno;
}

void usage()
{
	cout << "usage: mrsws [-c configfile] [-d datadir] [-p parserdir] [-i input] [-v] [-f]" << endl;
	cout << "    -c   config file to use, (default " MRS_CONFIG_FILE ")" << endl;
	cout << "    -d   data directory containing MRS files (default " MRS_DATA_DIR ")" << endl;
	cout << "    -p   parser directory containing parser scripts (default " MRS_PARSER_DIR ")" << endl;
	cout << "    -i   process command from input file and exit" << endl;
	cout << "    -f   stay in foreground (do not create a daemon)" << endl;
	cout << "    -v   be verbose" << endl;
	cout << endl;
	exit(1);
}

int main(int argc, char * const argv[])
{
	int c;
	string input_file, config_file, user;
	bool daemon = true;
	
	while ((c = getopt(argc, const_cast<char**>(argv), "c:d:p:i:vf")) != -1)
	{
		switch (c)
		{
			case 'c':
				gConfigFile = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'd':
				gDataDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'p':
				gParserDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'i':
				input_file = optarg;
				break;
			
			case 'v':
				++VERBOSE;
				break;
			
			case 'f':
				daemon = false;
				break;
			
			default:
				usage();
				break;
		}
	}
	
	// check the parameters
	
	if (fs::exists(gConfigFile))
	{
		gConfig = new WConfigFile(gConfigFile.string().c_str());
		
		string s;

		// user ID to use
		gConfig->GetSetting("/mrs-config/user", user);
		
		if (gConfig->GetSetting("/mrs-config/datadir", s))
			gDataDir = fs::system_complete(fs::path(s, fs::native));
		
		gStatusDir = gDataDir / ".." / "status";

		if (gConfig->GetSetting("/mrs-config/statusdir", s))
			gStatusDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/scriptdir", s))
			gParserDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/logfile", s))
			gLogFile = fs::system_complete(fs::path(s, fs::native));
	}
	else
	{
		cerr << "Configuration file " << gConfigFile.string() << " does not exist" << endl;
		exit(1);
	}
	
	if (not fs::exists(gDataDir) or not fs::is_directory(gDataDir))
	{
		cerr << "Data directory " << gDataDir.string() << " is not a valid directory" << endl;
		exit(1);
	}
	
	setenv("MRS_DATA_DIR", gDataDir.string().c_str(), 1);

	if (not fs::exists(gParserDir) or not fs::is_directory(gParserDir))
	{
		cerr << "Parser directory " << gParserDir.string() << " is not a valid directory" << endl;
		exit(1);
	}
	
	WFormatTable::SetParserDir(gParserDir.string());

	// --------------------------------------------------------------------
	// set up application and start Main Loop

	if (daemon)
		Daemonize(user);
	
	struct sigaction sa;
	
	sa.sa_handler = handler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
//	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGCHLD, &sa, NULL);

	gQuit = false;
	while (not gQuit)
	{
		gNeedReload = gConfig->ReloadIfModified() or gNeedReload;
		
		if (gNeedReload)
			OpenLogFile();
	
		RunMainLoop();
	}
	
	return 0;
}
