/*	$Id$
	Copyright Maarten L. Hekkelman
	Created 25-01-2007
*/

#include <iostream>
#include <algorithm>

#include "CJob.h"

using namespace std;

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
	: mStop(false)
{
	Start();
}

CJobQueue::~CJobQueue()
{
}

void CJobQueue::Submit(CJob* inJob)
{
	StMutex lock(mLock);

	mJobs.push_back(inJob);
	push_heap(mJobs.begin(), mJobs.end(), ComparePriority());
}

void CJobQueue::Run()
{
	while (not mStop)
	{
		CJob* job = NULL;

		{
			StMutex lock(mLock);

			if (mJobs.size() > 0)
			{
				job = mJobs.front();
				
				pop_heap(mJobs.begin(), mJobs.end(), ComparePriority());
				mJobs.erase(mJobs.end() - 1);
			}
		}
		
		if (job != NULL)
		{
			try
			{
				job->Execute();
			}
			catch (exception& e)
			{
				cerr << "Exception caught in CJobQueue::Run: " << e.what() << endl;
			}
			catch (...)
			{
				cerr << "Exception caught in CJobQueue::Run" << endl;
			}
		}
		else
			sleep(1);
	}
}

void CJobQueue::Stop()
{
	mStop = true;
	Join();
}
