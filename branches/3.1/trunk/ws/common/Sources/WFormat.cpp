/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Wednesday November 01 2006 13:58:37
	
	horror...
	
	The problem we face is that apparently the perl interpreter stores thread local data.
	And thus the format routines can only be called from the same thread that created
	the perl interpreter object. So we have to serialize access to the perl interpreter
	object which will now run in its own thread.
	
	But make sure the input and output is waited for in a decent manner.
	
	We are using three  mutexes to do this. One for locking access to the entire
	formatter object, one for notifying the format thread of the arrival of new data
	and one for the notification of the calling thread that data was processed.
	
	phew...

*/

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#undef do_open
#undef do_close

#include <string>
#include <deque>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

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

  private:

	void					Format();

	void					IndexName();

	void					Thread();

	boost::mutex			imp_mutex;
	boost::mutex			mutex;
	boost::condition		input;
	boost::condition		output;

	enum ECmd {
		eCmdStop,
		eCmdNoop,
		eCmdFormat,
		eCmdIxName
	}						cmd;
	string					formatter;
	string					format;
	string					text;
	string					db;
	string					id;
	string					result;

	PerlInterpreter*		my_perl;
	string					parser_dir;

	boost::thread*			thread;
};

WFormatTableImp::WFormatTableImp(
	const string&	inParserDir)
	: my_perl(NULL)
	, parser_dir(inParserDir)
	, thread(NULL)
{
	boost::mutex::scoped_lock lock(mutex);

	cmd = eCmdNoop;

	thread = new boost::thread(boost::bind(&WFormatTableImp::Thread, this));

	output.wait(lock);
}

WFormatTableImp::~WFormatTableImp()
{
	boost::mutex::scoped_lock lock(mutex);

	cmd = eCmdStop;
	input.notify_one();
	output.wait(lock);
	
	thread->join();
	delete thread;
}

void WFormatTableImp::Thread()
{	
	boost::mutex::scoped_lock lock(mutex);

	my_perl = perl_alloc();
	if (my_perl == NULL)
		THROW(("Error allocating perl interpreter"));
	
	perl_construct(my_perl);
	
	fs::path pd(parser_dir, fs::native);
	fs::path sp(pd / "WSFormatter.pm");

	char* embedding[] = { "", const_cast<char*>(sp.string().c_str()) };
	
	int err = perl_parse(my_perl, xs_init, 2, embedding, NULL);
	if (err != 0)
		THROW(("Error parsing embedded script: %d", err));
	
	err = perl_run(my_perl);
	if (err != 0)
		THROW(("Error parsing embedded script: %d", err));

	output.notify_one();

	bool stop = false;
	while (not stop)
	{
		input.wait(lock);
		
		switch (cmd)
		{
			case eCmdNoop:
				break;
			
			case eCmdStop:
				stop = true;
				break;
			
			case eCmdFormat:
				Format();
				break;
			
			case eCmdIxName:
				IndexName();
				break;
		}
		
		output.notify_one();
	}

	PL_perl_destruct_level = 0;
	perl_destruct(my_perl);
	perl_free(my_perl);
}

void WFormatTableImp::Format()
{
	dSP;                            /* initialize stack pointer      */
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
									/* push the parser directory name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(parser_dir.c_str(), parser_dir.length())));
									/* push the formatter name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(formatter.c_str(), formatter.length())));
									/* push the text onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(text.c_str(), text.length())));
									/* push the db onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(db.c_str(), db.length())));
									/* push the id onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(id.c_str(), id.length())));
	PUTBACK;						/* make local stack pointer global */

									/* call the function             */
	string call = "Embed::WSFormat::";
	call += format;
	perl_call_pv(call.c_str(), G_SCALAR | G_EVAL);

	SPAGAIN;                        /* refresh stack pointer         */
		                            /* pop the return value from stack */

	result = POPp;

	PUTBACK;
	FREETMPS;                       /* free that return value        */
	LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
}

void WFormatTableImp::IndexName()
{
	dSP;                            /* initialize stack pointer      */
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
									/* push the parser directory name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(parser_dir.c_str(), parser_dir.length())));
									/* push the formatter name onto the stack  */
	XPUSHs(sv_2mortal(newSVpvn(formatter.c_str(), formatter.length())));
									/* push the index name onto stack  */
	XPUSHs(sv_2mortal(newSVpvn(id.c_str(), id.length())));
	PUTBACK;						/* make local stack pointer global */

									/* call the function             */
	perl_call_pv("Embed::WSFormat::index_name", G_SCALAR | G_EVAL);

	SPAGAIN;                        /* refresh stack pointer         */
		                            /* pop the return value from stack */

	result = POPp;
	
	PUTBACK;
	FREETMPS;                       /* free that return value        */
	LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
}

string WFormatTableImp::Format(
	const string&	inFormatter,
	const string&	inFormat,
	const string&	inText,
	const string&	inDb,
	const string&	inId)
{
	boost::mutex::scoped_lock imp_lock(imp_mutex);
	boost::mutex::scoped_lock lock(mutex);

	cmd = eCmdFormat;
	
	formatter = inFormatter;
	format = inFormat;
	text = inText;
	db = inDb;
	id = inId;

	input.notify_one();
	output.wait(lock);

	return result;
}

string WFormatTableImp::IndexName(
	const string&	inFormatter,
	const string&	inIndex)
{
	boost::mutex::scoped_lock imp_lock(imp_mutex);
	boost::mutex::scoped_lock lock(mutex);

	cmd = eCmdIxName;
	
	formatter = inFormatter;
	id = inIndex;

	input.notify_one();
	output.wait(lock);

	return result;
}

// --------------------------------------------------------------------
//

string WFormatTable::sParserDir;

WFormatTable::WFormatTable()
{
}

WFormatTable::~WFormatTable()
{
}

WFormatTableImp* WFormatTable::Impl()
{
	static WFormatTableImp* sImpl = NULL;
	
	if (sImpl == NULL)
	{
		static boost::mutex sInitMutex;
		
		boost::mutex::scoped_lock lock(sInitMutex);
		
		if (sImpl == NULL)
		{
			fs::path pd(sParserDir, fs::native);
			if (not fs::exists(pd / "WSFormatter.pm"))
				THROW(("The WSFormatter.pm script cannot be found, it should be located in the parser scripts directory"));
	
			sImpl = new WFormatTableImp(sParserDir);
		}
	}
	
	return sImpl;
}

string WFormatTable::Format(
	const string&	inFormatter,
	const string&	inFormat,
	const string&	inText,
	const string&	inDb,
	const string&	inId)
{
	return Impl()->Format(inFormatter, inFormat, inText, inDb, inId);
}
	
string WFormatTable::IndexName(
	const string&	inFormatter,
	const string&	inIndex)
{
	return Impl()->IndexName(inFormatter, inIndex);
}

void WFormatTable::SetParserDir(
	const string&	inParserDir)
{
	if (inParserDir != sParserDir)
	{
		fs::path pd(inParserDir, fs::native);
		if (not fs::exists(pd / "WSFormatter.pm"))
			THROW(("The WSFormatter.pm script cannot be found, it should be located in the parser scripts directory"));
	
		sParserDir = inParserDir;
	}
}

