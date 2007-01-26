/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 25-01-2007
*/

#ifndef CJOB_H
#define CJOB_H

#include <deque>
#include <HMutex.h>
#include "CThread.h"

class CJob
{
  public:
						CJob();
	virtual				~CJob();
	
	virtual void		Execute() = 0;

  private:
						CJob(const CJob&);
	CJob&				operator=(const CJob&);
};

class CJobQueue : public CThread
{
  public:
						CJobQueue();
	virtual				~CJobQueue();
	
	void				Submit(CJob* inJob);

  private:

						CJobQueue(const CJobQueue&);
	CJobQueue&			operator=(const CJobQueue&);

	virtual void		Run();

	std::deque<CJob*>	mJobs;
	HMutex				mLock;
};

#endif
