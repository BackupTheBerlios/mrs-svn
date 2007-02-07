/*	$Id: mrsws-clustal.cpp$
	Copyright Maarten L. Hekkelman
	Created 23-01-2007
*/

#include <map>
#include <set>
#include <sstream>
#include <fstream>
#include <cstdarg>
#include <iomanip>
#include <sys/stat.h>
#include <signal.h>
#include <getopt.h>

#include "uuid/uuid.h"

#include <boost/shared_ptr.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "soapH.h"
#include "mrsws_clustal.nsmap"
#include "WError.h"
#include "WUtils.h"
#include "WConfig.h"

#define nil NULL

using namespace std;
namespace fs = boost::filesystem;

// default values for the directories used by mrsws, these can also be set from the Makefile

#ifndef MRS_CONFIG_FILE
#define MRS_CONFIG_FILE "/usr/local/etc/mrs-config.xml"
#endif

#ifndef MRS_LOG_FILE
#define MRS_LOG_FILE "/var/log/mrsws-clustal.log"
#endif

// globals

fs::path gConfigFile(MRS_CONFIG_FILE, fs::native);
fs::path gLogFile(MRS_LOG_FILE, fs::native);
fs::path gClustalWExe("clustalw", fs::native);
WConfigFile* gConfig;
int gVerbose;
int gMaxRunTime = 15;

extern double system_time();

// globals for communication with the outside world

bool	gQuit;
bool	gTimeOut;

void handler(int inSignal)
{
	int old_errno = errno;
	
	switch (inSignal)
	{
		case SIGINT:
			gQuit = true;
			break;
		
		case SIGALRM:
			gTimeOut = true;
			break;
		
		default:	// now what?
			break;
	}
	
	cout << "signal caught: " << inSignal << endl;
	
	errno = old_errno;
}

// --------------------------------------------------------------------
//
//	The real work
//

void RunClustalW(
	vector<struct ns__Sequence>		input,
	struct ns__ClustalWResponse&	response)
{
	// first create a fasta formatted 'file'
	
	stringstream fastaStream;
	
	for (vector<struct ns__Sequence>::iterator si = input.begin(); si != input.end(); ++si)
		fastaStream << '>' << si->id << endl << si->sequence << endl;
	
	string fasta = fastaStream.str();
	
	string path = gClustalWExe.string();

	char faFileName[] = "/tmp/clustal_input_XXXXXX";
	int fd = mkstemp(faFileName);
	
	if (fd < 0)
		THROW(("Failed to create fasta file: %s", strerror(fd)));
	
	ofstream fa(faFileName);
	fa << fasta;
	fa.close();
	
	close(fd);	// we're done with the input
	
	// ready to roll

	int ifd[2], ofd[2], efd[2];
	
	pipe(ifd);
	pipe(ofd);
	pipe(efd);
	
	int pid = fork();
	
	if (pid == -1)
	{
		close(ifd[0]);
		close(ifd[1]);
		close(ofd[0]);
		close(ofd[1]);
		close(efd[0]);
		close(efd[1]);
		
		THROW(("fork failed: %s", strerror(errno)));
	}
	
	// set a timer to avoid running clustal too long
	gTimeOut = false;
	struct itimerval it = {};
	it.it_value.tv_sec = gMaxRunTime;			// this is in seconds
	
	setitimer(ITIMER_REAL, &it, nil);
	
	if (pid == 0)	// the child
	{
		try
		{
			setpgid(0, 0);		// detach from the process group, create new

			dup2(ifd[0], STDIN_FILENO);
			close(ifd[0]);
			close(ifd[1]);

			dup2(ofd[1], STDOUT_FILENO);
			close(ofd[0]);
			close(ofd[1]);
							
			dup2(efd[1], STDERR_FILENO);
			close(efd[0]);
			close(efd[1]);

			string fileParam("-INFILE=");
			fileParam += faFileName;

			const char* argv[] = {
				path.c_str(),
				fileParam.c_str(),
				"-TYPE=PROTEIN",
				"-OUTPUT=GDE",
				"-CASE=UPPER",
				nil
			};
			
			if (gVerbose)
			{
				for (int i = 0; argv[i]; ++i)
					cout << "param " << i << ": " << argv[i] << endl;
			}
			
			const char* env[] = {
				nil
			};
			
			(void)execve(path.c_str(), const_cast<char* const*>(argv), const_cast<char* const*>(env));
			cerr << "execution of " << path << " failed: " << strerror(errno) << endl;
		}
		catch (const std::exception& e)
		{
			cerr << "Exception: " << e.what() << endl;
		}
		catch (...)
		{
			cerr << "Exception catched" << endl;
		}
		exit(-1);
	}
	
	close(ifd[0]);

	int flags;

	flags = fcntl(ifd[1], F_GETFL, 0);
	fcntl(ifd[1], F_SETFL, flags | O_NONBLOCK);
//		// since we're not using stdin:
//		close(ifd[1]);
	
	close(ofd[1]);
	flags = fcntl(ofd[0], F_GETFL, 0);
	fcntl(ofd[0], F_SETFL, flags | O_NONBLOCK);

	close(efd[1]);
	fcntl(ofd[0], F_GETFL, (int)&flags);
	fcntl(ofd[0], F_SETFL, flags | O_NONBLOCK);
	
	// OK, so now the executable is started and the pipes are set up
	// read from the pipes until done.
	
	bool errDone = false, outDone = false;
	
	while (not errDone and not outDone)
	{
		if (gTimeOut)
			kill(pid, SIGINT);
		
		char buffer[1024];
		int r, n;
		
		n = 0;
		while (not outDone)
		{
			r = read(ofd[0], buffer, sizeof(buffer));
			
			if (r > 0)
				response.diagnostics_out.append(buffer, r);
			else if (r == 0 or errno != EAGAIN)
				outDone = true;
			else
				break;
		}
		
		n = 0;
		while (not errDone)
		{
			r = read(efd[0], buffer, sizeof(buffer));
			
			if (r > 0)
				response.diagnostics_err.append(buffer, r);
			else if (r == 0 and errno != EAGAIN)
				errDone = true;
			else
				break;
		}
	}
	
	if (gVerbose)
	{
		cout << "stdout: " << endl << response.diagnostics_out << endl << endl;
		cout << "stderr: " << endl << response.diagnostics_err << endl << endl;
	}
	
	int status;
	waitpid(pid, &status, WNOHANG);
	
	it.it_value.tv_sec = 0;
	setitimer(ITIMER_REAL, &it, nil);
	
	string gdeFileName(faFileName);
//	gdeFileName.erase(sizeof(faFileName) - 3, 2);
	gdeFileName.append(".gde");
	
	ifstream gde(gdeFileName.c_str());
	if (gde.is_open())
	{
		string line;

		ns__Sequence s;
		boost::regex re("^%(\\S+)");
		
		for (;;)
		{
			getline(gde, line);

			if (gVerbose)
				cout << "gde: " << line << endl;
			
			if (gde.eof())
				break;
			
			if (line.length() == 0)
				continue;
			
			boost::smatch m;
			if (boost::regex_match(line, m, re))
			{
				if (s.id.length())
					response.alignment.push_back(s);
				s.id.assign(m[1].first, m[1].second);
				s.sequence.clear();
			}
			else
				s.sequence.append(line);
		}

		if (s.id.length())
			response.alignment.push_back(s);
	}
	
	try
	{
		// clean up the mess left behind by clustalw
		
		fs::remove(fs::path(faFileName, fs::native));
		fs::remove(fs::path(gdeFileName, fs::native));
		
		string dndFileName = gdeFileName.substr(0, gdeFileName.length() - 3) + "dnd";
		fs::remove(fs::path(dndFileName, fs::native));
	}
	catch (...)
	{
		// whatever...
	}
	
	if (gTimeOut)
		response.diagnostics_err += "ClustalW interrupted because the maximum runtime was exceeded";
}

// --------------------------------------------------------------------
//
//	SOAP calls
// 

SOAP_FMAC5 int SOAP_FMAC6
ns__ClustalW(
	struct soap*					soap,
	vector<struct ns__Sequence>		input,
	struct ns__ClustalWResponse&	response)
{
	WLogger log(soap->ip, __func__);
	
	int result = SOAP_OK;

	try
	{
		if (not fs::exists(gClustalWExe))
			THROW(("ClustalW executable %s does not exist", gClustalWExe.string().c_str()));
		
		RunClustalW(input, response);
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in clustal",
			e.what());
	}
	
	return result;
}

// --------------------------------------------------------------------
// 
//   main body
// 

void usage()
{
	cout << "usage: mrsws-clustal [-d datadir] [-p parserdir] [[-a address] [-p port] | -i input] [-v]" << endl;
	cout << "    -a   address to bind to (default localhost)" << endl;
	cout << "    -p   port number to bind to (default 8082)" << endl;
	cout << "    -i   process command from input file and exit" << endl;
	cout << "    -b   detach (daemon)" << endl;
	cout << "    -v   be verbose" << endl;
	cout << endl;
	exit(1);
}

int main(int argc, const char* argv[])
{
	int c;
	string input_file, address = "localhost", config_file;
	short port = 8082;
	bool daemon = false;
	
	while ((c = getopt(argc, const_cast<char**>(argv), "d:s:a:p:i:c:vbt:")) != -1)
	{
		switch (c)
		{
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
				++gVerbose;
				break;
			
			case 'b':
				daemon = true;
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
		
		if (gConfig->GetSetting("/mrs-config/clustal-ws/logfile", s))
			gLogFile = fs::system_complete(fs::path(s, fs::native));
	
		if (gConfig->GetSetting("/mrs-config/clustal-ws/clustalw-exe", s))
			gClustalWExe = fs::system_complete(fs::path(s, fs::native));
	
		if (gConfig->GetSetting("/mrs-config/clustal-ws/address", s))
			address = s;
	
		if (gConfig->GetSetting("/mrs-config/clustal-ws/port", s))
			port = atoi(s.c_str());

		if (gConfig->GetSetting("/mrs-config/clustal-ws/max-run-time", s))
			gMaxRunTime = atoi(s.c_str());
	}
	else if (gVerbose)
		cerr << "Configuration file " << gConfigFile.string() << " does not exist, ignoring" << endl;
	
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
		{
			logFile.open(gLogFile.string().c_str(), ios::out | ios::app);
			
			if (not logFile.is_open())
				cerr << "Opening log file " << gLogFile.string() << " failed" << endl;
			
			(void)cout.rdbuf(logFile.rdbuf());
			(void)cerr.rdbuf(logFile.rdbuf());

			int pid = fork();
			
			if (pid == -1)
			{
				cerr << "Fork failed" << endl;
				exit(1);
			}
			
			if (pid != 0)
			{
				cout << "Started daemon with process id: " << pid << endl;
				_exit(0);
			}
		}
		
		struct sigaction sa;
		
		sa.sa_handler = handler;
		sa.sa_flags = 0;
		sigemptyset(&sa.sa_mask);
		
		sigaction(SIGINT, &sa, NULL);
//		sigaction(SIGHUP, &sa, NULL);
		sigaction(SIGALRM, &sa, NULL);
			
		struct soap soap;
		int m, s; // master and slave sockets
		soap_init2(&soap, SOAP_IO_KEEPALIVE, SOAP_IO_KEEPALIVE|SOAP_C_UTFSTRING);
		
//	soap_set_recv_logfile(&soap, "recv.log"); // append all messages received in /logs/recv/service12.log
//	soap_set_sent_logfile(&soap, "sent.log"); // append all messages sent in /logs/sent/service12.log
//	soap_set_test_logfile(&soap, "test.log"); // no file name: do not save debug messages

		if (gVerbose)
			cout << "Binding address " << address << " port " << port << endl;

		m = soap_bind(&soap, address.c_str(), port, 100);
		if (m < 0)
			soap_print_fault(&soap, stderr);
		else
		{
			gQuit = false;
			
			soap.accept_timeout = 1;	// timeout
			
			for (;;)
			{
				if (gQuit)
					break;
				
				s = soap_accept(&soap);
				
				if (s == SOAP_EOF)
					continue;
				
				if (s < 0)
				{
					soap_print_fault(&soap, stderr);
					break;
				}
				
				try
				{
					if (soap_serve(&soap) != SOAP_OK) // process RPC request
;//						soap_print_fault(&soap, stderr); // print error
				}
				catch (const exception& e)
				{
					cout << endl << "Exception: " << e.what() << endl;
				}
				catch (...)
				{
					cout << endl << "Unknown exception" << endl;
				}
				
				soap_destroy(&soap); // clean up class instances
				soap_end(&soap); // clean up everything and close socket
			}
		}
		soap_done(&soap); // close master socket and detach environment
		
		cout << "Quit" << endl;
	}
	
	return 0;
}

