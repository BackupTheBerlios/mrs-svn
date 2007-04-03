/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 25-01-2007
*/

#include <algorithm>

#include "CJob.h"

struct ComparePriority
{
	bool operator()(const CJob* a, const CJob* b) const
		{ return a->Priority() < b->Priority(); }
};

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
		push_heap(mJobs.begin(), mJobs.end(), ComparePriority());
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
				
				pop_heap(mJobs.begin(), mJobs.end(), ComparePriority());
				mJobs.erase(mJobs.end() - 1);
			}
			
			mLock.Signal();
			
			if (job != NULL)
				job->Execute();
			else
				sleep(1);
		}
	}
}

