/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 13:58:37
*/

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#undef do_open
#undef do_close

#include <string>

#include "WFormat.h"
#include "WError.h"

using namespace std;

extern "C" {

void xs_init (pTHX);
void boot_DynaLoader (pTHX_ CV* cv);

void
xs_init(pTHX)
{
	char *file = __FILE__;
	dXSUB_SYS;

	/* DynaLoader is a special case */
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

}

struct WFormatTableImp
{
							WFormatTableImp();
							~WFormatTableImp();

	string					Format(
								const string&	inFormatDir,
								const string&	inFormatter,
								const string&	inFormat,
								const string&	inText,
								const string&	inDb,
								const string&	inId);
	
	PerlInterpreter*		my_perl;
};

WFormatTableImp::WFormatTableImp()
	: my_perl(NULL)
{
	my_perl = perl_alloc();
	if (my_perl == NULL)
		THROW(("Error allocating perl interpreter"));
	
	perl_construct(my_perl);

	char* embedding[] = { "", "WSFormatter.pm" };
	
	int err = perl_parse(my_perl, xs_init, 2, embedding, NULL);
	if (err != 0)
		THROW(("Error parsing embedded script: %d", err));
	
	err = perl_run(my_perl);
	if (err != 0)
		THROW(("Error parsing embedded script: %d", err));
}

WFormatTableImp::~WFormatTableImp()
{
	PL_perl_destruct_level = 0;
	perl_destruct(my_perl);
	perl_free(my_perl);
}

string WFormatTableImp::Format(
	const string& inFormatDir,
	const string& inFormatter,
	const string& inFormat,
	const string& inText,
	const string& inDb,
	const string& inId)
{
	dSP;                            /* initialize stack pointer      */
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
									/* push the format dir name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(inFormatDir.c_str(), inFormatDir.length())));
									/* push the formatter name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(inFormatter.c_str(), inFormatter.length())));
									/* push the text onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inText.c_str(), inText.length())));
									/* push the db onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inId.c_str(), inDb.length())));
									/* push the id onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inId.c_str(), inId.length())));
	PUTBACK;						/* make local stack pointer global */

									/* call the function             */
	string call = "Embed::WSFormat::";
	call += inFormat;
	perl_call_pv(call.c_str(), G_SCALAR | G_EVAL);

	SPAGAIN;                        /* refresh stack pointer         */
		                            /* pop the return value from stack */

	string result = POPp;
	
	PUTBACK;
	FREETMPS;                       /* free that return value        */
	LEAVE;                       /* ...and the XPUSHed "mortal" args.*/

	return result;
}

// --------------------------------------------------------------------
//

WFormatTable::WFormatTable()
	: mImpl(new WFormatTableImp())
{
}

WFormatTable::~WFormatTable()
{
	delete mImpl;
}

WFormatTable& WFormatTable::Instance()
{
	static WFormatTable sInstance;
	return sInstance;
}

string WFormatTable::Format(
	const string&	inFormatDir,
	const string&	inFormatter,
	const string&	inFormat,
	const string&	inText,
	const string&	inDb,
	const string&	inId)
{
	return mImpl->Format(inFormatDir, inFormatter, inFormat, inText, inDb, inId);
}
	
