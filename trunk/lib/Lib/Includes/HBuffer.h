/*	$Id: mrsws.cpp$
	Copyright Maarten L. Hekkelman
	Created Sunday January 05 2003 11:44:10
*/

#ifndef HBUFFER_H
#define HBUFFER_H

#include <deque>
#include <boost/thread.hpp>

template<class T, uint32 N = 100>
class HBuffer
{
  public:
	void				Put(
							T	inValue);

	T					Get();

	// flags to help debug performance issues
	bool				WasFull() const		{ return mWasFull; }
	bool				WasEmpty() const	{ return mWasEmpty; }

  private:
	std::deque<T>		mQueue;
	boost::mutex		mMutex;
	boost::condition	mEmptyCondition, mFullCondition;
	bool				mWasFull, mWasEmpty;
};

template<class T, uint32 N>
void HBuffer<T,N>::Put(
	T		inValue)
{
	boost::mutex::scoped_lock lock(mMutex);

	mWasFull = false;
	while (mQueue.size() >= N)
	{
		mFullCondition.wait(lock);
		mWasFull = true;
	}
	
	mQueue.push_back(inValue);

	mEmptyCondition.notify_one();
}

template<class T, uint32 N>
T HBuffer<T,N>::Get()
{
	boost::mutex::scoped_lock lock(mMutex);

	mWasEmpty = false;
	while (mQueue.empty())
	{
		mEmptyCondition.wait(lock);
		mWasEmpty = true;
	}
	
	T result = mQueue.front();
	mQueue.pop_front();

	mFullCondition.notify_one();
	
	return result;
}

#endif
