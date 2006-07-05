/*-
 * Copyright (c) 2005
 *      CMBI, Radboud University Nijmegen. All rights reserved.
 *
 * This code is derived from software contributed by Maarten L. Hekkelman
 * and Hekkelman Programmatuur b.v.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the Radboud University
 *        Nijmegen and its contributors.
 * 4. Neither the name of Radboud University nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "MRS.h"

#include <sys/types.h>
#include <sys/sem.h>
#include <pthread.h>

#include <iostream>

#include "HError.h"

#include "CSemaphore.h"
#include "CUtils.h"

using namespace std;

//#ifndef SEM_R
//#define SEM_FLGS	0600
//#else
//#define SEM_FLGS	(SEM_R | SEM_A)
//#endif
//
//CSemaphore::CSemaphore(int inCount)
//	: fSemID(semget(IPC_PRIVATE, 1, SEM_FLGS))
//{
//	if (fSemID < 0)
//		THROW(("Could not create semaphore"));
//	
//	struct sembuf sb = { 0, inCount, 0 };
//	if (semop(fSemID, &sb, 1) < 0)
//		THROW(("Could not update semaphore"));
//}
//
//CSemaphore::~CSemaphore()
//{
//	semctl(fSemID, 0, IPC_RMID);
//}
//
//bool CSemaphore::Wait()
//{
//	struct sembuf sb = { 0, -1, 0 };
//	if (semop(fSemID, &sb, 1) < 0)
//		THROW(("Could not get semaphore"));
//	
//	return true;
//}
//
//void CSemaphore::Signal()
//{
//	struct sembuf sb = { 0, 1, 0 };
//	if (semop(fSemID, &sb, 1) < 0)
//		THROW(("Could not release semaphore"));
//}
//
//#if P_DARWIN
//#include <libkern/OSAtomic.h>
//
//struct CBenaphoreImp
//{
//	int				fSemID;
//	int32_t			fCount;
//	
//					CBenaphoreImp()
//					{
//						fCount = 0;
//					}
//	
//	uint32			AtomicIncrement()
//					{
//						return ::OSAtomicIncrement32(&fCount);
//					}
//	uint32			AtomicDecrement()
//					{
//						return ::OSAtomicDecrement32(&fCount);
//					}
//};
//
//#elif P_LINUX
//#include <asm/atomic.h>
//
//struct CBenaphoreImp
//{
//	int				fSemID;
//	atomic_t		fCount;
//
//					CBenaphoreImp()
//					{
//						atomic_set(&fCount, 0);
//					}
//	
//	uint32			AtomicIncrement()
//					{
//						atomic_inc(&fCount);
//						return atomic_read(&fCount);
//					}
//	uint32			AtomicDecrement()
//					{
//						atomic_dec(&fCount);
//						return atomic_read(&fCount);
//					}
//};
//
//#else
//#	error
//#endif
//
//CBenaphore::CBenaphore()
//	: fImpl(new CBenaphoreImp)
//{
//	fImpl->fSemID = semget(IPC_PRIVATE, 1, SEM_FLGS);
//	
//	if (fImpl->fSemID < 0)
//		THROW(("Could not create semaphore"));
//	
//	struct sembuf sb = { 0, 0, 0 };
//	if (semop(fImpl->fSemID, &sb, 1) < 0)
//		THROW(("Could not update semaphore"));
//}
//
//CBenaphore::~CBenaphore()
//{
//	semctl(fImpl->fSemID, 0, IPC_RMID);
//	delete fImpl;
//}
//
//bool CBenaphore::Wait()
//{
//	if (fImpl->AtomicIncrement() > 1)
//	{
//		struct sembuf sb = { 0, -1, 0 };
//		if (semop(fImpl->fSemID, &sb, 1) < 0)
//			THROW(("Could not get semaphore"));
//	}
//	
//	return true;
//}
//
//void CBenaphore::Signal()
//{
//	if (fImpl->AtomicDecrement() > 0)
//	{
//		struct sembuf sb = { 0, 1, 0 };
//		if (semop(fImpl->fSemID, &sb, 1) < 0)
//			THROW(("Could not release semaphore"));
//	}
//}

struct CMutexImp
{
	pthread_mutex_t		m;
};

CMutex::CMutex()
	: _impl(new CMutexImp)
{
	if (pthread_mutex_init(&_impl->m, NULL) < 0)
		THROW(("Failed to create mutex"));
}

CMutex::~CMutex()
{
	pthread_mutex_destroy(&_impl->m);
	delete _impl;
}

bool CMutex::Wait()
{
	if (pthread_mutex_lock(&_impl->m) < 0)
		THROW(("Failed to lock mutex"));
	return true;
}

void CMutex::Signal()
{
	if (pthread_mutex_unlock(&_impl->m) < 0)
		THROW(("Failed to unlock mutex"));	
}
