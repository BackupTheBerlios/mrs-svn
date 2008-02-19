/*
	$Id: lock_and_run.cpp,v 1.2 2003/10/28 11:07:26 dbman Exp $
	
	lock and run is an app that tries to lock a lockfile and if
	it succeeds it runs the command that was specified. The exit
	code of the command is passed through to the calling app.
	
	This app is used to wrap commands like srscheck which cannot
	have more than one instance running at the same time.

	Updated to remove stale lockfiles.

	Created by: Maarten Hekkelman, CMBI
	Date:		14 juli 2003
*/

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <signal.h>

using namespace std;

// prototypes

void usage();
void error(const char* msg, ...);
int open_lock_file(string logfile);
void signal_handler(int inSignal);

// implementation

// use a class for lock_file

class lock_file
{
  public:
			lock_file(string inPath);
			~lock_file();

	void	disconnect();

  private:

	string	path;
};

lock_file::lock_file(string inPath)
	: path(inPath)
{
	int fd;
	FILE* f;
	
	// first check if there's a dangling lock file
	
	fd = open(path.c_str(), O_RDONLY);
	if (fd >= 0)
	{
		f = fdopen(fd, "r");
		if (f == NULL)
			error("fdopen failed: %s", strerror(errno));
		
		int pid;
		if (fscanf(f, "%d", &pid) == 1)
		{
			int err = kill(pid, SIGUSR1);
			if (err < 0 and errno == ESRCH)
			{
				err = unlink(path.c_str());
				if (err)
					error("lock file found, process seems to be dead but failed to remove lock file:\n%s\n",
						strerror(errno));
			}
		}
		
		fclose(f);
	}
	
	for (;;)
	{
		// loop until we succeed in creating the lock file
		
		fd = open(path.c_str(), O_RDWR | O_EXCL | O_CREAT, 0400);

		if (fd < 0 and errno == EEXIST)
		{
			sleep(random() % 8);
			continue;
		}
		
		if (fd < 0)
			error("Error creating lockfile: %s\n", strerror(errno));
		
		// if successful we write our pid to this file and close it
		
		f = fdopen(fd, "w");
		fprintf(f, "%d\n", getpid());
		
		fclose(f);
		break;
	}
}

lock_file::~lock_file()
{
	if (path.length())
		unlink(path.c_str());
}

void lock_file::disconnect()
{
	path.clear();
}

// Global

auto_ptr<lock_file>	gLockFile;

// other functions

void signal_handler(int inSignal)
{
	gLockFile.reset(NULL);
}

void error(const char* msg, ...)
{
	va_list vl;
	
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);
	puts("");
	
	exit(1);
}

void usage()
{
	puts("");
	puts("lock_and_run lockfile cmd ...");
	puts("");
	exit(1);
}


int main (int argc, /*const */char* argv[])
{
	if (argc < 3)
		usage();
	
	// install a signal handler to catch ^C
	
	struct sigaction act = { 0 };
	act.sa_handler = signal_handler;
	
	(void)sigaction(SIGTERM, &act, NULL);
	(void)sigaction(SIGINT, &act, NULL);

	act.sa_handler = SIG_IGN;
	(void)sigaction(SIGUSR1, &act, NULL);
	
	// create the lock file

	gLockFile.reset(new lock_file(argv[1]));
	
	// now fork a child
	
	pid_t pid = fork();

	if (pid < 0)
		error("Could not fork\n");
	
	if (pid == 0) // child process
	{
		// only the parent should delete a lock file
		gLockFile->disconnect();
		
		// execute the command
		execvp(argv[2], argv + 2);
		
		// should never get here:
		error("Could not lauch %s\n", argv[2]);
	}

	int result = 0, status = 1;
	if (waitpid(pid, &result, 0) == -1)
		error("waitpid failed: %s", strerror(errno));
	
	if (WIFEXITED(result))
		status = WEXITSTATUS(result);
	
	return status;
}
