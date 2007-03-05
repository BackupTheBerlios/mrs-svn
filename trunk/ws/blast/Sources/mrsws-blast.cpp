/*	$Id: mrsws-blast.cpp$
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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "soapH.h"
#include "mrsws_blast.nsmap"
#include "MRSInterface.h"
#include "CThread.h"
#include "WError.h"
#include "WFormat.h"
#include "WUtils.h"
#include "WConfig.h"
#include "CJob.h"

#define nil NULL

using namespace std;
namespace fs = boost::filesystem;

const double TIME_OUT_DATA = 3600;	// after an hour the data is purged from the cache
const unsigned long LONG_QUERY = 1850;

			// 95% of the sequences in SwissProt have a sequence length less than 1850
const unsigned long kShortLongQueueCutOff = 1850;

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
#define MRS_LOG_FILE "/var/log/mrsws-blast.log"
#endif

fs::path gDataDir(MRS_DATA_DIR, fs::native);
fs::path gParserDir(MRS_PARSER_DIR, fs::native);
fs::path gConfigFile(MRS_CONFIG_FILE, fs::native);
fs::path gLogFile(MRS_LOG_FILE, fs::native);

WConfigFile*	gConfig = nil;

// from mrs.h
extern unsigned int THREADS;
extern int VERBOSE;

extern double system_time();

struct CBlastJobParameters;
void DoBlast(
	const CBlastJobParameters&	inParams,
	vector<ns__Hit>&			outHits,
	unsigned long&				outDbCount,
	unsigned long&				outDbLength,
	unsigned long&				outEffectiveSpace,
	double&						outKappa,
	double&						outLambda,
	double&						outEntropy);

typedef boost::shared_ptr<MDatabank>	MDatabankPtr;

class WSDatabankTable
{
  public:
	typedef map<string,MDatabankPtr>	DBTable;
	typedef DBTable::const_iterator		iterator;

	static WSDatabankTable&	Instance();

	MDatabankPtr		operator[](const string& inCode);
	
	void				ReloadDbs();
	
	iterator			begin() const					{ return mDBs.begin(); }
	iterator			end() const						{ return mDBs.end(); }
	
  private:
	DBTable				mDBs;
};

WSDatabankTable& WSDatabankTable::Instance()
{
	static WSDatabankTable sInstance;
	return sInstance;
}

MDatabankPtr WSDatabankTable::operator[](const string& inCode)
{
	if (mDBs.find(inCode) == mDBs.end() or not mDBs[inCode]->IsUpToDate())
	{
		MDatabankPtr db(new MDatabank(inCode));
		db->PrefetchDocWeights("__ALL_TEXT__");
		mDBs[inCode] = db;
	}
	
	return mDBs[inCode];
}

void WSDatabankTable::ReloadDbs()
{
	mDBs.clear();
	
	cout << endl;
	
	DBInfoVector dbInfo;
	
	if (gConfig != nil and gConfig->GetSetting("/mrs-config/dbs/db", dbInfo))
	{
		for (DBInfoVector::iterator dbi = dbInfo.begin(); dbi != dbInfo.end(); ++dbi)
		{
			if (not dbi->blast)
				continue;
			
			fs::path f = gDataDir / (dbi->name + ".cmp");
				
			cout << "Loading " << dbi->name << " from " << f.string() << " ..."; cout.flush();
				
			try
			{
				MDatabankPtr db(new MDatabank(f.string()));
				db->PrefetchDocWeights("__ALL_TEXT__");
				mDBs[dbi->name] = db;
			}
			catch (exception& e)
			{
				cout << " failed" << endl;
				continue;
			}
			
			cout << " done" << endl;
		}
	}
	else
	{
		fs::directory_iterator end;
		for (fs::directory_iterator fi(gDataDir); fi != end; ++fi)
		{
			if (is_directory(*fi))
				continue;
			
			string name = fi->leaf();
			
			if (name.length() < 4 or name.substr(name.length() - 4) != ".cmp")
				continue;
			
			name.erase(name.length() - 4);
	
			cout << "Loading " << name << " from " << fi->string() << " ..."; cout.flush();
			
			try
			{
				MDatabankPtr db(new MDatabank(fi->string()));
//				db->PrefetchDocWeights("__ALL_TEXT__");
				mDBs[name] = db;
			}
			catch (exception& e)
			{
				cout << " failed" << endl;
				continue;
			}
			
			cout << " done" << endl;
		}
	}
}

//	-------------------------------------------------------------------
//
//	blast queue
//

struct CBlastJobParameters
{
						CBlastJobParameters(
							string			inDB,
							string			inQuery,
							string			inProgram,
							string			inMatrix,
							unsigned long	inWordSize,
							double			inExpect,
							bool			inLowComplexityFilter,
							bool			inGapped,
							unsigned long	inGapOpen,
							unsigned long	inGapExtend);

						CBlastJobParameters(
							const CBlastJobParameters& inOther);

	string				mDB;
	string				mQuery;
	string				mProgram;
	string				mMatrix;
	unsigned long		mWordSize;
	double				mExpect;
	bool				mLowComplexityFilter;
	bool				mGapped;
	unsigned long		mGapOpen;
	unsigned long		mGapExtend;
	
	bool				operator==(const CBlastJobParameters& inOther) const;
};

CBlastJobParameters::CBlastJobParameters(
	string			inDB,
	string			inQuery,
	string			inProgram,
	string			inMatrix,
	unsigned long	inWordSize,
	double			inExpect,
	bool			inLowComplexityFilter,
	bool			inGapped,
	unsigned long	inGapOpen,
	unsigned long	inGapExtend)
	: mDB(inDB)
	, mQuery(inQuery)
	, mProgram(inProgram)
	, mMatrix(inMatrix)
	, mWordSize(inWordSize)
	, mExpect(inExpect)
	, mLowComplexityFilter(inLowComplexityFilter)
	, mGapped(inGapped)
	, mGapOpen(inGapOpen)
	, mGapExtend(inGapExtend)
{
}

CBlastJobParameters::CBlastJobParameters(
	const CBlastJobParameters& inOther)
	: mDB(inOther.mDB)
	, mQuery(inOther.mQuery)
	, mProgram(inOther.mProgram)
	, mMatrix(inOther.mMatrix)
	, mWordSize(inOther.mWordSize)
	, mExpect(inOther.mExpect)
	, mLowComplexityFilter(inOther.mLowComplexityFilter)
	, mGapped(inOther.mGapped)
	, mGapOpen(inOther.mGapOpen)
	, mGapExtend(inOther.mGapExtend)
{
}

bool CBlastJobParameters::operator==(const CBlastJobParameters& inOther) const
{
	return
		mDB == inOther.mDB and
		mQuery == inOther.mQuery and
		mProgram == inOther.mProgram and
		mMatrix == inOther.mMatrix and
		mWordSize == inOther.mWordSize and
		mExpect == inOther.mExpect and
		mLowComplexityFilter == inOther.mLowComplexityFilter and
		mGapped == inOther.mGapped and
		mGapOpen == inOther.mGapOpen and
		mGapExtend == inOther.mGapExtend;
}

ostream& operator<<(ostream& inStream, const CBlastJobParameters& inParams)
{
	string query;
	
	if (inParams.mQuery.length() < 60)
		query = inParams.mQuery.length();
	else
		query = inParams.mQuery.substr(0, 57) + "...";
	
	inStream << endl << "params" << endl
			 << "db:                    " << inParams.mDB << endl
			 << "query:                 " << query << endl
			 << "program:               " << inParams.mProgram << endl
			 << "matrix:                " << inParams.mMatrix << endl
			 << "word_size:             " << inParams.mWordSize << endl
			 << "expect:                " << inParams.mExpect << endl
			 << "low_complexity_filter: " << inParams.mLowComplexityFilter << endl
			 << "gapped:                " << inParams.mGapped << endl
			 << "gap_open:              " << inParams.mGapOpen << endl
			 << "gap_extend:            " << inParams.mGapExtend << endl;

	return inStream;
}

class CBlastJob : public CJob
{
  public:
							
	static string		Create(
							const CBlastJobParameters& inParams);

	static CBlastJob*	Find(
							const string&	inID);

	ns__JobStatus		Status() const							{ return mStatus; }
	vector<ns__Hit>		Hits() const							{ mCollected = true; return mHits; }

	unsigned long		DbCount()								{ return mDbCount; }
	unsigned long		DbLength()								{ return mDbLength; }
	unsigned long		EffectiveSpace()						{ return mEffectiveSpace; }
	double				Kappa()									{ return mKappa; }
	double				Lambda()								{ return mLambda; }
	double				Entropy()								{ return mEntropy; }

	string				ID() const;
	string				Error() const							{ return mError; }

	static void			CheckCache(bool inPurge);
	
	virtual float		Priority() const						{ return 1.0 / mParams.mQuery.length(); }

  private:

	virtual void		Execute();

	static CBlastJob*	sHead;
	static HMutex		sLock;
//	static CJobQueue	sQueue;
	static CJobQueue	sShortQueue;
	static CJobQueue	sLongQueue;

						CBlastJob(
							const CBlastJobParameters& inParams);
//	virtual				~CBlastJob();

	uuid_t				mID;
	ns__JobStatus		mStatus;
	CBlastJob*			mNext;
	vector<ns__Hit>		mHits;

	unsigned long		mDbCount;
	unsigned long		mDbLength;
	unsigned long		mEffectiveSpace;
	double				mKappa;
	double				mLambda;
	double				mEntropy;

	mutable bool		mCollected;
	double				mCreated;
	double				mAccess;
	CBlastJobParameters	mParams;
	string				mError;
};

HMutex		CBlastJob::sLock;
CBlastJob*	CBlastJob::sHead = NULL;
//CJobQueue	CBlastJob::sQueue;
CJobQueue	CBlastJob::sShortQueue;
CJobQueue	CBlastJob::sLongQueue;

CBlastJob::CBlastJob(const CBlastJobParameters& inParams)
	: mStatus(unknown)
	, mNext(NULL)
	, mCollected(false)
	, mCreated(system_time())
	, mAccess(mCreated)
	, mParams(inParams)
{
	uuid_generate_time(mID);
}

string CBlastJob::Create(
	const CBlastJobParameters& inParams)
{
	StMutex lock(sLock);
	
	// first see if this job already exists in our cache
	unsigned long n = 0;
	CBlastJob* job = sHead;
	CBlastJob* last = NULL;
	
	while (job != NULL)
	{
		if (inParams == job->mParams)
			break;
		
		last = job;
		job = job->mNext;
		++n;
	}
	
	if (job != NULL)
		job->mAccess = system_time();
	else
	{
		job = new CBlastJob(inParams);
		
		if (sHead == NULL)
			sHead = job;
		else if (last != NULL)
			last->mNext = job;
		else
			assert(false);

		job->mStatus = queued;

//		sQueue.Submit(job);
		if (inParams.mQuery.length() >= LONG_QUERY)
			sLongQueue.Submit(job);
		else
			sShortQueue.Submit(job);
	}
	
	return job->ID();
}

CBlastJob* CBlastJob::Find(
	const string&	inID)
{
	uuid_t id;
	uuid_parse(inID.c_str(), id);
	
	StMutex lock(sLock);
	
	CBlastJob* job = sHead;
	
	while (job != NULL)
	{
		if (uuid_compare(id, job->mID) == 0)
			break;
		
		job = job->mNext;
	}
	
	if (job != NULL)
		job->mAccess = system_time();
	
	return job;
}

void CBlastJob::CheckCache(bool inPurge)
{
	StMutex lock(sLock);
	
	CBlastJob* job = sHead;
	CBlastJob* last = NULL;
	
	while (job != NULL)
	{
		if (job->mCollected and (inPurge or job->mAccess + TIME_OUT_DATA < system_time()))
		{
			CBlastJob* j = job;
			job = job->mNext;
			
			delete j;
			
			if (last != NULL)
				last->mNext = job;
			else
				sHead = job;
		}
		else
		{
			last = job;
			job = job->mNext;
		}
	}
}

void CBlastJob::Execute()
{
	try
	{
		mStatus = running;
		
		DoBlast(mParams, mHits, mDbCount, mDbLength, mEffectiveSpace, mKappa, mLambda, mEntropy);
		
		mStatus = finished;
	}
	catch (exception& e)
	{
		mStatus = error;
		mError = e.what();
	}
}

string CBlastJob::ID() const
{
	char id[128] = {};
	uuid_unparse(mID, id);
	return string(id);
}

// --------------------------------------------------------------------
//
//	Utility routines
// 

WLogger& operator<<(
	WLogger&					inLogger,
	const CBlastJobParameters&	inParams)
{
	string m = inParams.mDB + ' ';
	
	string q(inParams.mQuery);
	if (q.length() > 20)
		q = q.substr(0, 17) + "...";
	
	m += q;
	m += ' ';
	
	return inLogger << m;
}

// --------------------------------------------------------------------
//
//	The real work
//

void DoBlast(
	const CBlastJobParameters&	inParams,
	vector<ns__Hit>&			outHits,
	unsigned long&				outDbCount,
	unsigned long&				outDbLength,
	unsigned long&				outEffectiveSpace,
	double&						outKappa,
	double&						outLambda,
	double&						outEntropy)
{
	if (VERBOSE)
		cout << inParams << endl;
	
	MDatabankPtr mrsDb = WSDatabankTable::Instance()[inParams.mDB];

	auto_ptr<MBlastHits> hits(mrsDb->Blast(inParams.mQuery, inParams.mMatrix,
		inParams.mWordSize, inParams.mExpect, inParams.mLowComplexityFilter,
		inParams.mGapped, inParams.mGapOpen, inParams.mGapExtend));

	if (hits.get() != NULL)
	{
		outDbCount = hits->DbCount();
		outDbLength = hits->DbLength();
		outEffectiveSpace = hits->EffectiveSpace();
		outKappa = hits->Kappa();
		outLambda = hits->Lambda();
		outEntropy = hits->Entropy();
		
		for (auto_ptr<MBlastHit> hit(hits->Next()); hit.get() != NULL; hit.reset(hits->Next()))
		{
			ns__Hit h;
			
			h.id = hit->Id();
			h.title = hit->Title();
			
			auto_ptr<MBlastHsps> hsps(hit->Hsps());

			for (auto_ptr<MBlastHsp> hsp(hsps->Next()); hsp.get() != NULL; hsp.reset(hsps->Next()))
			{
				ns__Hsp hs;
				
				hs.score = hsp->Score();
				hs.bit_score = hsp->BitScore();
				hs.expect = hsp->Expect();
				hs.query_start = hsp->QueryStart();
				hs.subject_start = hsp->SubjectStart();
				hs.identity = hsp->Identity();
				hs.positive = hsp->Positive();
				hs.gaps = hsp->Gaps();
				hs.subject_length = hsp->SubjectLength();
				hs.query_alignment = hsp->QueryAlignment();
				hs.subject_alignment = hsp->SubjectAlignment();
				hs.midline = hsp->Midline();
				
				h.hsps.push_back(hs);
			}
			
			outHits.push_back(h);
		}
	}
}

// --------------------------------------------------------------------
//
//	SOAP calls
// 

SOAP_FMAC5 int SOAP_FMAC6
ns__BlastSync(
	struct soap*						soap,
	string								db,
	string								query,
	string								program,
	string								matrix,
	unsigned long						word_size,
	double								expect,
	bool								low_complexity_filter,
	bool								gapped,
	unsigned long						gap_open,
	unsigned long						gap_extend,
	struct ns__BlastResult&				response)
{
	WLogger log(soap->ip, __func__);

	int result = SOAP_OK;
	
	try
	{
		if (query.length() <= word_size)
			THROW(("Query sequence too short"));
		
		CBlastJobParameters params(db, query, program, matrix, word_size,
			expect, low_complexity_filter, gapped, gap_open, gap_extend);
		
		log << params;
		
		DoBlast(params, response.hits,
			response.db_count, response.db_length, response.eff_space,
			response.kappa, response.lambda, response.entropy);
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in blast",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__BlastAsync(
	struct soap*						soap,
	string								db,
	string								query,
	string								program,
	string								matrix,
	unsigned long						word_size,
	double								expect,
	bool								low_complexity_filter,
	bool								gapped,
	unsigned long						gap_open,
	unsigned long						gap_extend,
	xsd__string&						response)
{
	WLogger log(soap->ip, __func__);

	int result = SOAP_OK;
	
	try
	{
		if (query.length() <= word_size)
			THROW(("Query sequence too short"));
		
		CBlastJobParameters params(db, query, program, matrix, word_size,
			expect, low_complexity_filter, gapped, gap_open, gap_extend);

		log << params;
		
		response = CBlastJob::Create(params);
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in blast",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__BlastJobStatus(
	struct soap*					soap,
	xsd__string						job_id,
	enum ns__JobStatus&				response)
{
	WLogger log(soap->ip, __func__);

	int result = SOAP_OK;
	
	try
	{
		log << job_id;

		CBlastJob* job = CBlastJob::Find(job_id);
		
		if (job == NULL)
			THROW(("Unknown job id %s", job_id.c_str()));
		
		response = job->Status();
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in blast",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__BlastJobResult(
	struct soap*					soap,
	xsd__string						job_id,
	struct ns__BlastResult&			response)
{
	WLogger log(soap->ip, __func__);

	int result = SOAP_OK;
	
	try
	{
		log << job_id;

		CBlastJob* job = CBlastJob::Find(job_id);
		
		if (job == NULL)
			THROW(("Unknown job id %s", job_id.c_str()));

		if (job->Status() != finished)
			THROW(("Job %s not finished yet", job_id.c_str()));
		
		response.db_count = job->DbCount();
		response.db_length = job->DbLength();
		response.eff_space = job->EffectiveSpace();
		response.kappa = job->Kappa();
		response.lambda = job->Lambda();
		response.entropy = job->Entropy();
		
		response.hits = job->Hits();
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in blast",
			e.what());
	}

	return result;
}

SOAP_FMAC5 int SOAP_FMAC6
ns__BlastJobError(
	struct soap*					soap,
	xsd__string						job_id,
	xsd__string&					response)
{
	WLogger log(soap->ip, __func__);

	int result = SOAP_OK;
	
	try
	{
		log << job_id;

		CBlastJob* job = CBlastJob::Find(job_id);
		
		if (job == NULL)
			THROW(("Unknown job id %s", job_id.c_str()));
		
		response = job->Error();;
	}
	catch (exception& e)
	{
		return soap_receiver_fault(soap,
			"An error occurred in blast",
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
	cout << "usage: mrsws-blast [-d datadir] [-p parserdir] [[-a address] [-p port] | -i input] [-v]" << endl;
	cout << "    -d   data directory containing MRS files (default " << gDataDir.string() << ')' << endl;
	cout << "    -p   parser directory containing parser scripts (default " << gParserDir.string() << ')' << endl;
	cout << "    -a   address to bind to (default localhost)" << endl;
	cout << "    -p   port number to bind to (default 8082)" << endl;
	cout << "    -i   process command from input file and exit" << endl;
	cout << "    -b   detach (daemon)" << endl;
	cout << "    -t   nr of threads" << endl;
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
			
			case 'b':
				daemon = true;
				break;
			
			case 't':
				THREADS = atoi(optarg);
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
		
		if (gConfig->GetSetting("/mrs-config/datadir", s))
			gDataDir = fs::system_complete(fs::path(s, fs::native));
		
		if (gConfig->GetSetting("/mrs-config/scriptdir", s))
		{
			gParserDir = fs::system_complete(fs::path(s, fs::native));
			WFormatTable::SetParserDir(gParserDir.string());
		}
		
		if (gConfig->GetSetting("/mrs-config/blast-ws/logfile", s))
			gLogFile = fs::system_complete(fs::path(s, fs::native));
	
		if (gConfig->GetSetting("/mrs-config/blast-ws/address", s))
			address = s;
	
		if (gConfig->GetSetting("/mrs-config/blast-ws/port", s))
			port = atoi(s.c_str());

		if (gConfig->GetSetting("/mrs-config/blast-ws/threads", s))
			THREADS = atoi(s.c_str());
	}
	else if (VERBOSE)
		cerr << "Configuration file " << gConfigFile.string() << " does not exist, ignoring" << endl;
	
	if (not fs::exists(gDataDir) or not fs::is_directory(gDataDir))
	{
		cerr << "Data directory " << gDataDir.string() << " is not a valid directory" << endl;
		exit(1);
	}
	
	if (not fs::exists(gParserDir) or not fs::is_directory(gParserDir))
	{
		cerr << "Parser directory " << gParserDir.string() << " is not a valid directory" << endl;
		exit(1);
	}

	WFormatTable::SetParserDir(gParserDir.string());
	
	if (input_file.length())
	{
		struct soap soap;
		
		soap_init(&soap);

		WSDatabankTable::Instance().ReloadDbs();
					
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

		m = soap_bind(&soap, address.c_str(), port, 100);
		if (m < 0)
			soap_print_fault(&soap, stderr);
		else
		{
			gQuit = false;
			gNeedReload = true;
			
			soap.accept_timeout = 1;	// timeout
			soap.recv_timeout = 10;
			soap.send_timeout = 10;
			soap.max_keep_alive = 10;
			
			for (;;)
			{
				if (gQuit)
					break;
				
				CBlastJob::CheckCache(gNeedReload);

				if (gNeedReload or gConfig->ReloadIfModified())
					WSDatabankTable::Instance().ReloadDbs();

				gNeedReload = false;
				
				s = soap_accept(&soap);
				
				if (s == SOAP_EOF)
					continue;
				
				if (s < 0)
				{
					soap_print_fault(&soap, stderr);
					continue;
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

