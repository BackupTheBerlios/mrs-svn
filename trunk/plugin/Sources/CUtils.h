/*	$Id: CUtils.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Monday December 30 2002 14:34:43
*/

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
 
#ifndef CUTILS_H
#define CUTILS_H

#include <string>
#include <ctime>
#include <typeinfo>

class ProgressCallBackHandlerBase
{
  public:
	virtual			~ProgressCallBackHandlerBase() {}

	virtual void	ProgressCallBack(int inFase, float inComplete) = 0;

	static void		Tick(ProgressCallBackHandlerBase* inProgress,
						int inFase, float inComplete);

  private:
	static std::time_t	sLastTick;
};

inline
void ProgressCallBackHandlerBase::Tick(ProgressCallBackHandlerBase* inProgress,
	int inFase, float inComplete)
{
	if (inProgress)
	{
		std::time_t now;
		std::time(&now);
		if (inComplete == 1.0 || now > sLastTick)
		{
			inProgress->ProgressCallBack(inFase, inComplete);
			sLastTick = now;
		}
	}
}

template <class T>
class ProgressCallBackHandler : public ProgressCallBackHandlerBase
{
	typedef void	(T::*ProgressCallBackProc)(int inFase, float inComplete);
	
  public:
					ProgressCallBackHandler(T* inHandler, ProgressCallBackProc inMethod)
						: handler(inHandler), method(inMethod) {}

	virtual void	ProgressCallBack(int inFase, float inComplete)
						{ assert(handler); assert(method); (handler->*method)(inFase, inComplete); }
	
  private:
	T*						handler;
	ProgressCallBackProc	method;
};

std::string basename(const std::string& inName);
std::string tolower(const std::string& inName);

#endif // CUTILS_H
