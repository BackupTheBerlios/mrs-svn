/*	$Id: CLexicon.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Tuesday February 17 2004 10:41:41
	
	CLexicon efficiently stores unique strings which can be
	accessed later by the id returned by the method Store.
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
 
#ifndef CLEXICON_H
#define CLEXICON_H

#include <string>
#include <vector>

class CLexCompare
{
  public:
	virtual		~CLexCompare() {}

	virtual int	Compare(
					const char*				inA,
					uint32					inALength,
					const char*				inB,
					uint32					inBLength) const = 0;
};

class CLexicon
{
	friend struct CLexPage;
	
  public:
					CLexicon();

	virtual			~CLexicon();
	
	// CLexicon is one of the most heavily used classes
	// To optimize, we now use separate threads but there
	// can only be one lexicon. So we need locking.
	// Profiling has shown that locking is taking a
	// disproportionate amount of time, so we split the
	// code into a part that looks up words first in a
	// shared lock mode and then stores those unknown
	// in a unique lock mode.
	
	void			LockShared();
	
	void			UnlockShared();
	
	void			LockUnique();
	
	void			UnlockUnique();
	
	uint32			Lookup(
						const std::string&	inWord) const;

	uint32			Store(
						const std::string&	inWord);

	uint32			Lookup(
						const char*			inWord,
						uint32				inWordLength) const;

	uint32			Store(
						const char*			inWord,
						uint32				inWordLength);

	std::string		GetString(
						uint32				inNr) const;

	void			GetString(
						uint32				inNr,
						const char*&		outWord,
						uint32&				outWordLength) const;

	int				Compare(
						uint32				inA,
						uint32				inB) const;
	
	int				Compare(
						uint32				inA,
						uint32				inB,
						CLexCompare&		inCompare) const;

	uint32			Count() const;
	
  private:
					CLexicon(
						const CLexicon&);

	CLexicon&		operator=(
						const CLexicon&);
	
	struct CLexiconImp*	fImpl;
};

inline
uint32 CLexicon::Lookup(
	const std::string&	inWord) const
{
	return Lookup(inWord.c_str(), inWord.length());
}

inline
uint32 CLexicon::Store(
	const std::string&	inWord)
{
	return Store(inWord.c_str(), inWord.length());
}

inline
std::string CLexicon::GetString(
	uint32				inNr) const
{
	const char* w;
	uint32 l;

	GetString(inNr, w, l);

	return std::string(w, l);
}

#endif // CLEXICON_H
