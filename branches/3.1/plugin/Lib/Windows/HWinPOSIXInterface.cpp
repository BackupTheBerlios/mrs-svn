/*	$Id: HWinPOSIXInterface.cpp 18 2006-03-01 15:31:09Z hekkel $
	Copyright Hekkelman Programmatuur b.v.
	Created By Bas Vodde on Saturday January 12 2002 16:16:22
*/

#include "HLib.h"

/*
	BAS TODO: 
	Implementation copied from X and doesn't looks finished.
	BAS TODO: WINDOWS DOES NOT THREAD SOCKETS AND FILES THE SAME!!!
	fcntl doesn't seem to exists but care must be taken if its about
	a file or about a socket!
*/

#include "HPOSIXInterface.h"
#include "HGlobals.h"
#include "HNativeTypes.h"

namespace HPosix
{

char* getcwd(char* path, long path_len)
{
	*path = 0;
	return path;
}

}
