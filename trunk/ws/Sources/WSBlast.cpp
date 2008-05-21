#include "HLib.h"

#include <set>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include "uuid/uuid.h"

#include "HError.h"
#include "MRSInterface.h"

#include "WConfig.h"
#include "WSBlast.h"
#include "WFormat.h"

#include "WSBlastNS.nsmap"

using namespace std;

extern double system_time();

// --------------------------------------------------------------------
//
//	SOAP calls
// 

namespace WSBlastNS {

int ns__BlastSync(
	struct soap*			soap,
	string					db,
	string					mrsBooleanQuery,
	string					query,
	string					program,
	string					matrix,
	unsigned long			word_size,
	double					expect,
	bool					low_complexity_filter,
	bool					gapped,
	unsigned long			gap_open,
	unsigned long			gap_extend,
	unsigned long			report_limit,
	struct ns__BlastResult&	response)
{
	return WSBlast::GetServer(soap)->BlastSync(db, mrsBooleanQuery,
		query, program, matrix, word_size, expect,
		low_complexity_filter, gapped, gap_open, gap_extend, report_limit,
		response);
}

int ns__BlastAsync(
	struct soap*			soap,
	string					db,
	string					mrsBooleanQuery,
	string					query,
	string					program,
	string					matrix,
	unsigned long			word_size,
	double					expect,
	bool					low_complexity_filter,
	bool					gapped,
	unsigned long			gap_open,
	unsigned long			gap_extend,
	unsigned long			report_limit,
	string&					response)
{
	return WSBlast::GetServer(soap)->BlastAsync(db, mrsBooleanQuery,
		query, program, matrix, word_size, expect,
		low_complexity_filter, gapped, gap_open, gap_extend, report_limit,
		response);
}

int ns__BlastJobStatus(
	struct soap*			soap,
	string					job_id,
	enum ns__JobStatus&		response)
{
	return WSBlast::GetServer(soap)->BlastJobStatus(job_id, response);
}

int ns__BlastJobResult(
	struct soap*			soap,
	string					job_id,
	struct ns__BlastResult&	response)
{
	return WSBlast::GetServer(soap)->BlastJobResult(job_id, response);
}

int ns__BlastJobError(
	struct soap*			soap,
	string					job_id,
	string&					response)
{
	return WSBlast::GetServer(soap)->BlastJobError(job_id, response);
}

}

// --------------------------------------------------------------------
//

struct CBlastJobParameters
{
						CBlastJobParameters(
							string			inDB,
							string			inMrsBooleanQuery,
							string			inQuery,
							string			inProgram,
							string			inMatrix,
							unsigned long	inWordSize,
							double			inExpect,
							bool			inLowComplexityFilter,
							bool			inGapped,
							unsigned long	inGapOpen,
							unsigned long	inGapExtend,
							unsigned long	inReportLimit);

						CBlastJobParameters(
							const CBlastJobParameters& inOther);

	string				mDB;
	string				mMrsBooleanQuery;
	string				mQuery;
	string				mProgram;
	string				mMatrix;
	unsigned long		mWordSize;
	double				mExpect;
	bool				mLowComplexityFilter;
	bool				mGapped;
	unsigned long		mGapOpen;
	unsigned long		mGapExtend;
	unsigned long		mReportLimit;
	
	bool				operator==(const CBlastJobParameters& inOther) const;
};


CBlastJobParameters::CBlastJobParameters(
	string			inDB,
	string			inMrsBooleanQuery,
	string			inQuery,
	string			inProgram,
	string			inMatrix,
	unsigned long	inWordSize,
	double			inExpect,
	bool			inLowComplexityFilter,
	bool			inGapped,
	unsigned long	inGapOpen,
	unsigned long	inGapExtend,
	unsigned long	inReportLimit)
	: mDB(inDB)
	, mMrsBooleanQuery(inMrsBooleanQuery)
	, mQuery(inQuery)
	, mProgram(inProgram)
	, mMatrix(inMatrix)
	, mWordSize(inWordSize)
	, mExpect(inExpect)
	, mLowComplexityFilter(inLowComplexityFilter)
	, mGapped(inGapped)
	, mGapOpen(inGapOpen)
	, mGapExtend(inGapExtend)
	, mReportLimit(inReportLimit)
{
}

CBlastJobParameters::CBlastJobParameters(
	const CBlastJobParameters& inOther)
	: mDB(inOther.mDB)
	, mMrsBooleanQuery(inOther.mMrsBooleanQuery)
	, mQuery(inOther.mQuery)
	, mProgram(inOther.mProgram)
	, mMatrix(inOther.mMatrix)
	, mWordSize(inOther.mWordSize)
	, mExpect(inOther.mExpect)
	, mLowComplexityFilter(inOther.mLowComplexityFilter)
	, mGapped(inOther.mGapped)
	, mGapOpen(inOther.mGapOpen)
	, mGapExtend(inOther.mGapExtend)
	, mReportLimit(inOther.mReportLimit)
{
}

bool CBlastJobParameters::operator==(const CBlastJobParameters& inOther) const
{
	return
		mDB == inOther.mDB and
		mMrsBooleanQuery == inOther.mMrsBooleanQuery and
		mQuery == inOther.mQuery and
		mProgram == inOther.mProgram and
		mMatrix == inOther.mMatrix and
		mWordSize == inOther.mWordSize and
		mExpect == inOther.mExpect and
		mLowComplexityFilter == inOther.mLowComplexityFilter and
		mGapped == inOther.mGapped and
		mGapOpen == inOther.mGapOpen and
		mGapExtend == inOther.mGapExtend and
		mReportLimit == inOther.mReportLimit;
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
			 << "mrs-boolean-query:     " << inParams.mMrsBooleanQuery << endl
			 << "query:                 " << query << endl
			 << "program:               " << inParams.mProgram << endl
			 << "matrix:                " << inParams.mMatrix << endl
			 << "word_size:             " << inParams.mWordSize << endl
			 << "expect:                " << inParams.mExpect << endl
			 << "low_complexity_filter: " << inParams.mLowComplexityFilter << endl
			 << "gapped:                " << inParams.mGapped << endl
			 << "gap_open:              " << inParams.mGapOpen << endl
			 << "gap_extend:            " << inParams.mGapExtend << endl
			 << "report_limit:          " << inParams.mReportLimit << endl;

	return inStream;
}

// --------------------------------------------------------------------
//

class CBlastJob;
typedef boost::shared_ptr<CBlastJob>	CBlastJobPtr;

class CBlastJob
{
  public:
						CBlastJob(
							const CBlastJobParameters& inParams,
							WSBlast*					inServer);
	
	virtual				~CBlastJob() {}
							
	void				Execute();

	const CBlastJobParameters&
						Params() const							{ return mParams; }

	WSBlastNS::ns__JobStatus
						Status() const							{ return mStatus; }

	vector<WSBlastNS::ns__Hit>
						Hits() const							{ mCollected = true; return mHits; }

	unsigned long		DbCount()								{ return mDbCount; }
	unsigned long		DbLength()								{ return mDbLength; }
	unsigned long		EffectiveSpace()						{ return mEffectiveSpace; }
	double				Kappa()									{ return mKappa; }
	double				Lambda()								{ return mLambda; }
	double				Entropy()								{ return mEntropy; }

	string				ID() const;
	string				Error() const							{ return mError; }

	virtual float		Priority() const						{ return 1.0 / mParams.mQuery.length(); }

  private:

	WSBlast*			mServer;

	uuid_t				mID;
	WSBlastNS::ns__JobStatus		mStatus;
	CBlastJob*			mNext;
	vector<WSBlastNS::ns__Hit>		mHits;

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

CBlastJob::CBlastJob(
	const CBlastJobParameters&		inParams,
	WSBlast*						inServer)
	: mServer(inServer)
	, mStatus(WSBlastNS::unknown)
	, mNext(NULL)
	, mCollected(false)
	, mCreated(system_time())
	, mAccess(mCreated)
	, mParams(inParams)
{
	uuid_generate_time(mID);
}

//CBlastJobPtr CBlastJob::Create(
//	const CBlastJobParameters& inParams)
//{
//	StMutex lock(sLock);
//	
//	// first see if this job already exists in our cache
//	unsigned long n = 0;
//	CBlastJob* job = sHead;
//	CBlastJob* last = NULL;
//	
//	while (job != NULL)
//	{
//		if (inParams == job->mParams)
//			break;
//		
//		last = job;
//		job = job->mNext;
//		++n;
//	}
//	
//	if (job != NULL)
//		job->mAccess = system_time();
//	else
//	{
//		job = new CBlastJob(inParams);
//		
//		if (sHead == NULL)
//			sHead = job;
//		else if (last != NULL)
//			last->mNext = job;
//		else
//			assert(false);
//
//		job->mStatus = queued;
//
//		if (sQueue == NULL)
//			sQueue = new CJobQueue;
//
//		sQueue->Submit(job);
////		if (inParams.mQuery.length() >= LONG_QUERY)
////			sLongQueue->Submit(job);
////		else
////			sShortQueue->Submit(job);
//	}
//	
//	return job->ID();
//}
//
//CBlastJob* CBlastJob::Find(
//	const string&	inID)
//{
//	uuid_t id;
//	uuid_parse(inID.c_str(), id);
//	
//	StMutex lock(sLock);
//	
//	CBlastJob* job = sHead;
//	
//	while (job != NULL)
//	{
//		if (uuid_compare(id, job->mID) == 0)
//			break;
//		
//		job = job->mNext;
//	}
//	
//	if (job != NULL)
//		job->mAccess = system_time();
//	
//	return job;
//}
//
//void CBlastJob::CheckCache(bool inPurge)
//{
//	StMutex lock(sLock);
//	
//	CBlastJob* job = sHead;
//	CBlastJob* last = NULL;
//	
//	while (job != NULL)
//	{
//		if (job->mCollected and (inPurge or job->mAccess + TIME_OUT_DATA < system_time()))
//		{
//			CBlastJob* j = job;
//			job = job->mNext;
//			
//			delete j;
//			
//			if (last != NULL)
//				last->mNext = job;
//			else
//				sHead = job;
//		}
//		else
//		{
//			last = job;
//			job = job->mNext;
//		}
//	}
//}

void CBlastJob::Execute()
{
	try
	{
		mStatus = WSBlastNS::running;
		
		mServer->DoBlast(mParams, mHits, mDbCount, mDbLength, mEffectiveSpace, mKappa, mLambda, mEntropy);
		
		mStatus = WSBlastNS::finished;
	}
	catch (exception& e)
	{
		mStatus = WSBlastNS::error;
		mError = e.what();
	}
}

string CBlastJob::ID() const
{
	char id[128] = {};
	uuid_unparse(mID, id);
	return string(id);
}

//// --------------------------------------------------------------------
////
////	Utility routines
//// 
//
//ostream& operator<<(
//	ostream&					inLogger,
//	const CBlastJobParameters&	inParams)
//{
//	string m = inParams.mDB + ' ';
//	
//	string q(inParams.mQuery);
//	if (q.length() > 20)
//		q = q.substr(0, 17) + "...";
//	
//	m += q;
//	m += ' ';
//	
//	return inLogger << m;
//}

// --------------------------------------------------------------------
//
//	The real work
//

void WSBlast::DoBlast(
	const CBlastJobParameters&	inParams,
	vector<WSBlastNS::ns__Hit>&	outHits,
	unsigned long&				outDbCount,
	unsigned long&				outDbLength,
	unsigned long&				outEffectiveSpace,
	double&						outKappa,
	double&						outLambda,
	double&						outEntropy)
{
	if (VERBOSE)
		cout << inParams << endl;
	
	MDatabankPtr mrsDb = mDBs[inParams.mDB];

	auto_ptr<MBlastHits> hits;
	
	if (inParams.mMrsBooleanQuery.length() > 0)
	{
		auto_ptr<MQueryResults> qr(mrsDb->Find(inParams.mMrsBooleanQuery));
		if (qr.get() != nil)
		{
			hits.reset(qr->Blast(inParams.mQuery, inParams.mMatrix,
				inParams.mWordSize, inParams.mExpect, inParams.mLowComplexityFilter,
				inParams.mGapped, inParams.mGapOpen, inParams.mGapExtend, inParams.mReportLimit));
		}
	}
	else
		hits.reset(mrsDb->Blast(inParams.mQuery, inParams.mMatrix,
			inParams.mWordSize, inParams.mExpect, inParams.mLowComplexityFilter,
			inParams.mGapped, inParams.mGapOpen, inParams.mGapExtend, inParams.mReportLimit));

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
			WSBlastNS::ns__Hit h;
			
			h.id = hit->Id();
			h.title = hit->Title();
			
			auto_ptr<MBlastHsps> hsps(hit->Hsps());

			for (auto_ptr<MBlastHsp> hsp(hsps->Next()); hsp.get() != NULL; hsp.reset(hsps->Next()))
			{
				WSBlastNS::ns__Hsp hs;
				
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
//	Server implementation
// 

WSBlast::WSBlast(
	const string&	inAddress,
	uint16			inPort,
	DBInfoVector&	inDbs)
	: WServerT<WSBlast>(inAddress, inPort, WSBlastNS_namespaces)
	, mDBs(inDbs)
	, mStopBlasting(false)
	, mLoopThread(boost::bind(&WSBlast::Loop, this))
{
}

WSBlast::~WSBlast()
{
	mStopBlasting = true;
	mLoopThread.join();
}

int WSBlast::BlastSync(
	string					db,
	string					mrsBooleanQuery,
	string					query,
	string					program,
	string					matrix,
	unsigned long			word_size,
	double					expect,
	bool					low_complexity_filter,
	bool					gapped,
	unsigned long			gap_open,
	unsigned long			gap_extend,
	unsigned long			report_limit,
	struct WSBlastNS::ns__BlastResult&
							response)
{
	THROW(("BlastSync is currently not supported, please use BlastAsync instead"));
	
//	if (query.length() <= word_size)
//		THROW(("Query sequence too short"));
//	
//	CBlastJobParameters params(db, mrsBooleanQuery, query, program, matrix,
//		word_size, expect, low_complexity_filter, gapped, gap_open, gap_extend, report_limit);
//	
//	Log() << params;
//	
//	DoBlast(params, response.hits,
//		response.db_count, response.db_length, response.eff_space,
//		response.kappa, response.lambda, response.entropy);
//	
//	return SOAP_OK;
}

int WSBlast::BlastAsync(
	string					db,
	string					mrsBooleanQuery,
	string					query,
	string					program,
	string					matrix,
	unsigned long			word_size,
	double					expect,
	bool					low_complexity_filter,
	bool					gapped,
	unsigned long			gap_open,
	unsigned long			gap_extend,
	unsigned long			report_limit,
	string&					response)
{
	CBlastJobParameters params(db, mrsBooleanQuery, query, program, matrix,
		word_size, expect, low_complexity_filter, gapped, gap_open, gap_extend, report_limit);
	
	Log() << params;
	
	SubmitJob(params);
	
	return SOAP_OK;
}

int WSBlast::BlastJobStatus(
	string					job_id,
	enum WSBlastNS::ns__JobStatus&
							response)
{
	Log() << job_id;

	CBlastJobPtr job(GetJob(job_id));
	response = job->Status();

	return SOAP_OK;
}

int WSBlast::BlastJobResult(
	string					job_id,
	struct WSBlastNS::ns__BlastResult&
							response)
{
	Log() << job_id;

	CBlastJobPtr job = GetJob(job_id);
	
	if (job->Status() != WSBlastNS::finished)
		THROW(("Job %s not finished yet", job_id.c_str()));
	
	response.db_count = job->DbCount();
	response.db_length = job->DbLength();
	response.eff_space = job->EffectiveSpace();
	response.kappa = job->Kappa();
	response.lambda = job->Lambda();
	response.entropy = job->Entropy();
	
	response.hits = job->Hits();

	return SOAP_OK;
}

int WSBlast::BlastJobError(
	string					job_id,
	string&					response)
{
	Log() << job_id;
	
	CBlastJobPtr job = GetJob(job_id);
	response = job->Error();
	
	return SOAP_OK;
}

CBlastJobPtr WSBlast::SubmitJob(
	const CBlastJobParameters&	params)
{
	boost::mutex::scoped_lock lock(mQueueMutex);

	CBlastJobList::iterator j = find_if(mCache.begin(), mCache.end(),
		boost::bind(&CBlastJob::Params, _1) == params);
	
	CBlastJobPtr result;
	
	if (j == mCache.end())
	{
		CBlastJobPtr job(new CBlastJob(params, this));
	
		mCache.push_back(job);
		
		mQueue.push_back(job);
		push_heap(mQueue.begin(), mQueue.end(),
			boost::bind(&CBlastJob::Priority, _1) < boost::bind(&CBlastJob::Priority, _2));
		
		mQueueEmpty.notify_one();
		
		result = job;
	}
	else
		result = *j;
	
	return result;
}

CBlastJobPtr WSBlast::GetJob(
	const std::string&			job_id)
{
	boost::mutex::scoped_lock lock(mQueueMutex);

	CBlastJobList::iterator j = find_if(mCache.begin(), mCache.end(),
		boost::bind(&CBlastJob::ID, _1) == job_id);
	
	if (j == mCache.end())
		THROW(("Job not found: %s", job_id.c_str()));
	
	return *j;
}

void WSBlast::Loop()
{
	while (not mStopBlasting)
	{
		boost::mutex::scoped_lock lock(mQueueMutex);
		mQueueEmpty.wait(lock);
	
		if (not mQueue.empty())
		{
			pop_heap(mQueue.begin(), mQueue.end(),
				boost::bind(&CBlastJob::Priority, _1) < boost::bind(&CBlastJob::Priority, _2));
			
			CBlastJobPtr job = mQueue.back();
			mQueue.erase(mQueue.end() - 1);
	
			lock.unlock();	// unlock the mutex first

			job->Execute();
		}
	}
}

int WSBlast::Serve(
	struct soap*			inSoap)
{
	return WSBlastNS::WSBlastNS_serve_request(inSoap);
}

void WSBlast::Stop()
{
	mStopBlasting = true;
	WServer::Stop();
}
