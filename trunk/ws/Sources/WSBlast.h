#ifndef WSBLAST_H
#define WSBLAST_H

#include "WServer.h"
#include "WSBlastNSH.h"

#include <boost/shared_ptr.hpp>

struct CBlastJobParameters;
class CBlastJob;

typedef boost::shared_ptr<CBlastJob> CBlastJobPtr;

class WSBlast : public WServerT<WSBlast>
{
  public:
						WSBlast(
							const std::string&			inAddress,
							uint16						inPortNr,
							std::vector<std::string>&	inDbs);

	virtual				~WSBlast();
	
	int					BlastSync(
							std::string					db,
							std::string					mrsBooleanQuery,
							std::string					query,
							std::string					program,
							std::string					matrix,
							unsigned long				word_size,
							double						expect,
							bool						low_complexity_filter,
							bool						gapped,
							unsigned long				gap_open,
							unsigned long				gap_extend,
							unsigned long				report_limit,
							struct WSBlastNS::ns__BlastResult&
														response);
		
	int					BlastAsync(
							std::string					db,
							std::string					mrsBooleanQuery,
							std::string					query,
							std::string					program,
							std::string					matrix,
							unsigned long				word_size,
							double						expect,
							bool						low_complexity_filter,
							bool						gapped,
							unsigned long				gap_open,
							unsigned long				gap_extend,
							unsigned long				report_limit,
							std::string&				response);
		
	int					BlastJobStatus(
							std::string					job_id,
							enum WSBlastNS::ns__JobStatus&
														response);
						
	int					BlastJobResult(
							std::string					job_id,
							struct WSBlastNS::ns__BlastResult&
														response);
					
	int					BlastJobError(
							std::string					job_id,
							std::string&				response);
	
	void				DoBlast(
							const CBlastJobParameters&	inParams,
							std::vector<WSBlastNS::ns__Hit>&
														outHits,
							unsigned long&				outDbCount,
							unsigned long&				outDbLength,
							unsigned long&				outEffectiveSpace,
							double&						outKappa,
							double&						outLambda,
							double&						outEntropy);

	virtual void		Stop();

  protected:

	virtual int			Serve(
							struct soap*				inSoap);
	
	void				Loop();

	typedef std::vector<CBlastJobPtr>					CBlastJobList;

	CBlastJobPtr		SubmitJob(
							const CBlastJobParameters&	params);

	CBlastJobPtr		GetJob(
							const std::string&			job_id);

	std::vector<std::string>
						mDBs;
	CBlastJobList		mQueue;
	CBlastJobList		mCache;
	boost::mutex		mQueueMutex;
	boost::condition	mQueueEmpty;
	bool				mStopBlasting;
	boost::thread		mLoopThread;
};

#endif
