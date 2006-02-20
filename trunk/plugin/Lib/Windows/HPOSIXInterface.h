/*	$Id: HPOSIXInterface.h,v 1.5 2003/02/27 13:19:48 maarten Exp $
	Copyright Hekkelman Programmatuur b.v.
	Created By Bas Vodde on Saturday January 12 2002 16:15:17
*/

#ifndef HPOSIXINTERFACE_H
#define HPOSIXINTERFACE_H

//#include <sys/types.h>
//#include <sys/wait.h>
//#include <unistd.h>

#if ! P_WIN
#include <sys/stat.h>
#endif
//#include <semaphore.h>

#define	HS_IRWXU    S_IRWXU			/* RWX mask for owner */
#define	HS_IRUSR    S_IRUSR			/* R for owner */
#define	HS_IWUSR    S_IWUSR			/* W for owner */
#define	HS_IXUSR    S_IXUSR			/* X for owner */
                           
#define	HS_IRWXG    S_IRWXG			/* RWX mask for group */
#define	HS_IRGRP    S_IRGRP			/* R for group */
#define	HS_IWGRP    S_IWGRP			/* W for group */
#define	HS_IXGRP    S_IXGRP			/* X for group */
                           
#define	HS_IRWXO    S_IRWXO			/* RWX mask for other */
#define	HS_IROTH    S_IROTH			/* R for other */
#define	HS_IWOTH    S_IWOTH			/* W for other */
#define	HS_IXOTH    S_IXOTH			/* X for other */

namespace HPosix
{
	typedef struct stat HStat;
	
	int fork(void);
	int pipe(int fd[2]);
	int setpgid(int pid, int pgid);
	int close(int fd);
	int dup2(int fd1, int fd2);
	int execve(const char* path, char* const argv[], char* const* envp);
	int read(int fd, void* buf, unsigned int nbyte);
	int write(int fd, void* buf, unsigned int nbyte);
//	int waitpid(pid_t pid, int* status_loc, int options);
	int errno_();
	int kill(int pid, int signal);
	int chdir(const char* path);
	int getuid();
	int getgid();
	int fcntl(int fd, int cmd, int arg);
	int stat(const char* inPath, HStat& outStat);
	int chown(const char* inPath, long inUid, long inGid);
	int chmod(const char* inPath, long inMode);
	int umask(int inMask);
	int sem_post(const char* inSem);
	char* getcwd(char* path, long path_len);
}

#endif // HPOSIXINTERFACE
