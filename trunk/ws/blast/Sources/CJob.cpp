/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 25-01-2007
*/

#include "CJob.h"

CJob::CJob()
{
}

CJob::~CJob()
{
}

CJobQueue::CJobQueue()
{
	Start();
}

CJobQueue::~CJobQueue()
{
}

void CJobQueue::Submit(CJob* inJob)
{
	if (mLock.Wait())
	{
		mJobs.push_back(inJob);
		mLock.Signal();
	}
}

void CJobQueue::Run()
{
	for (;;)
	{
		if (mLock.Wait())
		{
			CJob* job = NULL;
			if (mJobs.size() > 0)
			{
				job = mJobs.front();
				mJobs.pop_front();
			}
			
			mLock.Signal();
			
			if (job != NULL)
				job->Execute();
			else
				sleep(1);
		}
	}
}

