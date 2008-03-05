/*	$Id: mrsws.cpp$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#include "HLib.h"

#include <map>
#include <set>
#include <deque>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <iomanip>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/array.hpp>

#include "soapH.h"
#include "mrsws_admin.nsmap"
#include "WError.h"
#include "WUtils.h"
#include "WConfig.h"
#include "WBuffer.h"

#define nil NULL

using namespace std;
namespace fs = boost::filesystem;

// default values for the directories used by mrsws, these can also be set from the Makefile

#ifndef MRS_DATA_DIR
#define MRS_DATA_DIR "/usr/local/data/mrs/"
#endif

#ifndef MRS_PARSER_DIR
#define MRS_PARSER_DIR "/usr/local/share/mrs/parser_scripts/"
#endif

#ifndef MRS_CONFIG_FILE
#define MRS_CONFIG_FILE "/usr/local/etc/mrs-config.xml"
#endif

#ifndef MRS_LOG_FILE
#define MRS_LOG_FILE "/var/log/mrsws-admin.log"
#endif

#ifndef MRS_PID_FILE
#define MRS_PID_FILE "/var/run/mrsws-admin.pid"
#endif

fs::path gDataDir(MRS_DATA_DIR, fs::native);
fs::path gParserDir(MRS_PARSER_DIR, fs::native);
fs::path gConfigFile(MRS_CONFIG_FILE, fs::native);
fs::path gLogFile(MRS_LOG_FILE, fs::native);

WConfigFile*	gConfig = nil;
int				VERBOSE = 0;

extern double system_time();

// local types

typedef map<string,ns__DatabankStatusInfo>	DBStatusTable;

DBStatusTable gStatus;

// --------------------------------------------------------------------
//
//	SOAP calls
// 

SOAP_FMAC5 int SOAP_FMAC6
ns__GetDatabankStatusInfo(
	struct soap*		soap,
	string				db,
	vector<struct ns__DatabankStatusInfo>&
						response)
{
	int result = SOAP_OK;
	WLogger log(soap->ip, __func__);

	try
	{
		log << db;
		
		if (db == "all")
		{
			for (DBStatusTable::iterator di = gStatus.begin(); di != gStatus.end(); ++di)
				response.push_back(di->second);
		}
		else
		{
			DBStatusTable::iterator di = gStatus.find(db);
			if (di != gStatus.end())
				response.push_back(di->second);
			else
				THROW(("Databank not found: %s", db.c_str()));
		}
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in function GetDatabankStatusInfo",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetDatabankMirrorLog(
	struct soap*		soap,
	string				db,
	unsigned long		age,
	string&				response)
{
	int result = SOAP_OK;
	WLogger log(soap->ip, __func__);

	try
	{
		
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in function GetDatabankMirrorLog",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetDatabankMakeLog(
	struct soap*		soap,
	string				db,
	unsigned long		age,
	string&				response)
{
	int result = SOAP_OK;
	WLogger log(soap->ip, __func__);

	try
	{
		
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in function GetDatabankMakeLog",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__GetParserScript(
	struct soap*		soap,
	string				script,
	string&				response)
{
	int result = SOAP_OK;
	WLogger log(soap->ip, __func__);

	try
	{
		
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in function GetParserScript",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__SetDatabankStatusInfo(
	struct soap*		soap,
	string				db,
	enum ns__DatabankStatus
						status,
	float				progress,
	string				message,
	int&				response)
{
	int result = SOAP_OK;
	WLogger log(soap->ip, __func__);

	try
	{
		log << db << " "
			<< message;

		struct ns__DatabankStatusInfo info;

		info.name = db;
		info.status = status;
		info.progress = progress;
		info.message = message;

		gStatus[db] = info;
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in function SetDatabankStatusInfo",
			e.what());
	}

	return result;
}

// --------------------------------------------------------------------
// 
//   main body
// 

// globals for communication with the outside world

bool	gNeedReload;
bool	gQuit;

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
	cout << "usage: mrsws [-c configfile] [-d datadir] [-p parserdir] [[-a address] [-p port] | -i input] [-v]" << endl;
	cout << "    -c   config file to use, (default " MRS_CONFIG_FILE ")" << endl;
	cout << "    -d   data directory containing MRS files (default " << gDataDir.string() << ')' << endl;
	cout << "    -p   parser directory containing parser scripts (default " << gParserDir.string() << ')' << endl;
	cout << "    -a   address to bind to (default localhost)" << endl;
	cout << "    -p   port number to bind to (default 8081)" << endl;
	cout << "    -i   process command from input file and exit" << endl;
	cout << "    -f   stay in foreground, do not daemonize" << endl;
	cout << "    -v   be verbose" << endl;
	cout << endl;
	exit(1);
}

int main(int argc, const char* argv[])
{
	int c;
	string input_file, address = "localhost", config_file, user;
	short port = 8081;
	bool daemon = true;
	
	while ((c = getopt(argc, const_cast<char**>(argv), "d:s:a:p:i:c:vf")) != -1)
	{
		switch (c)
		{
			case 'd':
				gDataDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 's':
				gParserDir = fs::system_complete(fs::path(optarg, fs::native));
				break;
			
			case 'a':
				address = optarg;
				break;
			
			case 'p':
				port = atoi(optarg);
				break;
			
			case 'i':
				input_file = optarg;
				break;
			
			case 'c':
				gConfigFile = fs::system_complete(fs::path(optarg, fs::native));
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
		
		if (gConfig->GetSetting("/mrs-config/scriptdir", s))
			gParserDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/admin-ws/logfile", s))
			gLogFile = fs::system_complete(fs::path(s, fs::native));
	
		if (gConfig->GetSetting("/mrs-config/admin-ws/address", s))
			address = s;
	
		if (gConfig->GetSetting("/mrs-config/admin-ws/port", s))
			port = atoi(s.c_str());
	}
	else if (VERBOSE)
		cerr << "Configuration file " << gConfigFile.string() << " does not exist, ignoring" << endl;
	
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

	if (input_file.length())
	{
		struct soap soap;
		
		soap_init(&soap);

		soap_serve(&soap);
		soap_destroy(&soap);
		soap_end(&soap);
	}
	else
	{
		ofstream logFile;
		
		if (daemon)
			Daemonize(user, gLogFile.string(), MRS_PID_FILE, logFile);
		
		struct sigaction sa;
		
		sa.sa_handler = handler;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		
		sigaction(SIGINT, &sa, NULL);
		sigaction(SIGHUP, &sa, NULL);
		sigaction(SIGPIPE, &sa, NULL);
		
		struct soap soap;
		int m, s; // master and slave sockets
		soap_init2(&soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE|SOAP_C_UTFSTRING);

//	soap_set_recv_logfile(&soap, "recv.log"); // append all messages received in /logs/recv/service12.log
//	soap_set_sent_logfile(&soap, "sent.log"); // append all messages sent in /logs/sent/service12.log
//	soap_set_test_logfile(&soap, "test.log"); // no file name: do not save debug messages

		if (VERBOSE)
			cout << "Binding address " << address << " port " << port << endl;

		// enable reuse of our address
		soap.bind_flags = SO_REUSEADDR;

		m = soap_bind(&soap, address.c_str(), port, 100);
		if (m < 0)
			soap_print_fault(&soap, stderr);
		else
		{
			gQuit = false;
			
			while (not gQuit)
			{
				soap.accept_timeout = 5;	// timeout
				soap.recv_timeout = 30;
				soap.send_timeout = 30;
				soap.max_keep_alive = 10;

				s = soap_accept(&soap);
				if (s == SOAP_EOF)
					continue;

				if (s < 0)
				{
					if (soap.errnum != 0)
						soap_print_fault(&soap, stderr);

					continue;
				}

				if (soap_serve(&soap) != SOAP_OK and soap.errnum != 0)
					soap_print_fault(&soap, stderr);
				
				soap_destroy(&soap);
				soap_end(&soap);
			}
		}

		soap_done(&soap); // close master socket and detach environment
		
		cout << "Quit" << endl;
	}
	
	return 0;
}

