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
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "WFormat.h"
#include "WError.h"

using namespace std;
namespace fs = boost::filesystem;

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
							WFormatTableImp(
								const string&	inParserDir);

							~WFormatTableImp();

	string					Format(
								const string&	inFormatter,
								const string&	inFormat,
								const string&	inText,
								const string&	inDb,
								const string&	inId);

	string					IndexName(
								const string&	inFormatter,
								const string&	inIndex);
	
	PerlInterpreter*		my_perl;
	string					parser_dir;
};

WFormatTableImp::WFormatTableImp(
	const string&	inParserDir)
	: my_perl(NULL)
	, parser_dir(inParserDir)
{
	my_perl = perl_alloc();
	if (my_perl == NULL)
		THROW(("Error allocating perl interpreter"));
	
	perl_construct(my_perl);
	
	fs::path pd(inParserDir, fs::native);
	fs::path sp(pd / "WSFormatter.pm");

	char* embedding[] = { "", const_cast<char*>(sp.string().c_str()) };
	
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
									/* push the parser directory name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(parser_dir.c_str(), parser_dir.length())));
									/* push the formatter name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(inFormatter.c_str(), inFormatter.length())));
									/* push the text onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inText.c_str(), inText.length())));
									/* push the db onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inDb.c_str(), inDb.length())));
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

string WFormatTableImp::IndexName(
	const string& inFormatter,
	const string& inIndex)
{
	dSP;                            /* initialize stack pointer      */
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
									/* push the parser directory name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(parser_dir.c_str(), parser_dir.length())));
									/* push the formatter name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(inFormatter.c_str(), inFormatter.length())));
									/* push the index name onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(inIndex.c_str(), inIndex.length())));
	PUTBACK;						/* make local stack pointer global */

									/* call the function             */
	perl_call_pv("Embed::WSFormat::index_name", G_SCALAR | G_EVAL);

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
	: mImpl(NULL)
{
}

WFormatTable::~WFormatTable()
{
	delete mImpl;
}

WFormatTable& WFormatTable::Instance()
{
	static WFormatTable sInstance;
	
	if (sInstance.mImpl == NULL)
	{
		fs::path pd(sInstance.mParserDir, fs::native);
		if (not fs::exists(pd / "WSFormatter.pm"))
			THROW(("The WSFormatter.pm script cannot be found, it should be located in the parser scripts directory"));
		
		sInstance.mImpl = new WFormatTableImp(sInstance.mParserDir);
	}

	return sInstance;
}

string WFormatTable::Format(
	const string&	inFormatter,
	const string&	inFormat,
	const string&	inText,
	const string&	inDb,
	const string&	inId)
{
	return mImpl->Format(inFormatter, inFormat, inText, inDb, inId);
}
	
string WFormatTable::IndexName(
	const string&	inFormatter,
	const string&	inIndex)
{
	return mImpl->IndexName(inFormatter, inIndex);
}

void WFormatTable::SetParserDir(
	const string&	inParserDir)
{
	if (inParserDir != mParserDir)
	{
		fs::path pd(inParserDir, fs::native);
		if (not fs::exists(pd / "WSFormatter.pm"))
			THROW(("The WSFormatter.pm script cannot be found, it should be located in the parser scripts directory"));
	
		delete mImpl;
		mImpl = NULL;
		
		mParserDir = inParserDir;
	}
}

