/*	$Id: mrsws.cpp$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#ifndef WBUFFER_H
#define WBUFFER_H

#include <boost/thread.hpp>

template<class T, uint32 N = 10>
class WBuffer
{
  public:
	void				Put(
							T	inValue);

	T					Get();

  private:
	deque<T>			mQueue;
	boost::mutex		mMutex;
	boost::condition	mEmptyCondition, mFullCondition;
};

template<class T, uint32 N>
void WBuffer<T,N>::Put(
	T		inValue)
{
	boost::mutex::scoped_lock lock(mMutex);

	while (mQueue.size() >= N)
		mFullCondition.wait(lock);
	
	mQueue.push_back(inValue);

	mEmptyCondition.notify_one();
}

template<class T, uint32 N>
T WBuffer<T,N>::Get()
{
	boost::mutex::scoped_lock lock(mMutex);

	while (mQueue.empty())
		mEmptyCondition.wait(lock);
	
	T result = mQueue.front();
	mQueue.pop_front();

	mFullCondition.notify_one();
	
	return result;
}

#endif
