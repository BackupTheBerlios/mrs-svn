/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 25-01-2007
*/

#ifndef CJOB_H
#define CJOB_H

#include <vector>
#include <HMutex.h>
#include "CThread.h"

class CJob
{
  public:
						CJob();
	virtual				~CJob();
	
	virtual void		Execute() = 0;

							// for priority queue's, range should be between 1 (high) and 0 (low)	
	virtual float		Priority() const 	{ return 1; }

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
	
	bool				Empty();

  private:

						CJobQueue(const CJobQueue&);
	CJobQueue&			operator=(const CJobQueue&);

	virtual void		Run();

	std::vector<CJob*>	mJobs;
	HMutex				mLock;
};

#endif
