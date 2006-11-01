/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 13:58:37
*/

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

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

//	/* DynaLoader is a special case */
//	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file);
}

}

struct WFormatTableImp
{
							WFormatTableImp();
							~WFormatTableImp();

	string					Format(const string& inParser, const string& inText);
	
	PerlInterpreter*		my_perl;
};

WFormatTableImp::WFormatTableImp()
	: my_perl(NULL)
{
	my_perl = perl_alloc();
	if (my_perl == NULL)
		THROW(("Error allocating perl interpreter"));
	
	perl_construct(my_perl);

	char* embedding[] = { "", "WSFormatter.pl" };
	
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

string WFormatTableImp::Format(const string& inParser, const string& inText)
{
	char* args[] = {
		const_cast<char*>(inParser.c_str()),
		const_cast<char*>(inText.c_str()),
		NULL
	};

	dSP;                            /* initialize stack pointer      */

	int c = perl_call_argv("Embed::WSFormat::pretty", G_EVAL | G_SCALAR, args);
	
	SV* retval = POPs;
	
	if (c != 1)
		THROW(("pretty print did not return a value"));

	if (not SvPOK(retval))
		THROW(("pretty print did not return a string"));
	
	string result = SvPV(retval, PL_na);
	
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
	const string&	inParser,
	const string&	inText)
{
	return mImpl->Format(inParser, inText);
}
	
