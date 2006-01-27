/*
 * $Id: MRS_swig.h,v 1.39 2005/10/11 13:17:31 maarten Exp $
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
 
#ifndef MRS_SWIG_H
#define MRS_SWIG_H

#include "HStlString.h"
#include "HStlVector.h"

class MDatabank;
typedef std::vector<MDatabank*> MDatabankArray;

std::string errstr();

// a few BLAST helper functions

std::string DUST(const std::string& inSequence);
std::string SEG(const std::string& inSequence);

// some globals, settable from the outside
extern int VERBOSE;
extern unsigned int THREADS;
extern const char* COMPRESSION;
extern int COMPRESSION_LEVEL;
extern const char* COMPRESSION_DICTIONARY;
extern std::string gErrStr;

class MBlastHits;
class MBlastHsps;
class MStringIterator;
class MIndices;
class MIndex;
class MQueryObject;
class MQueryResults;

struct MDatabankImp;
struct MQueryObjectImp;
struct MQueryResultsImp;
struct MKeysImp;
struct MIndexImp;
struct MIndicesImp;
struct MBlastHitImp;
struct MBlastHitsImp;
struct MBlastHspImp;
struct MBlastHspsImp;

// base class for all our interface objects
template<class Derived, class Impl>
class MRSObject
{
  public:
	virtual				~MRSObject();

	static Derived*		Create(Impl* inImpl)
							{ Derived* result = new Derived; result->fImpl = inImpl; return result; }

  protected:
	typedef Impl		impl;
	
						MRSObject()
							: fImpl(NULL) {}

						MRSObject(impl* inImpl)
							: fImpl(inImpl) {}

	impl*				GetImpl()			{ return fImpl; }

	impl*				fImpl;

  private:

						MRSObject(const MRSObject&);
	MRSObject&			operator=(MRSObject&);
};

class MStringIterator
{
  public:
						MStringIterator(std::vector<std::string>& inStrings)
							: fStrings(inStrings)
							, fIter(fStrings.begin()) {}
	
	const char*			Next()
						{
							const char* result = NULL;
		
							if (fIter != fStrings.end())
							{
								result = fIter->c_str();
								++fIter;
							}
							
							return result;
						}

  private:
						MStringIterator(const MStringIterator&);
	MStringIterator&	operator=(const MStringIterator&);

	std::vector<std::string>				fStrings;
	std::vector<std::string>::iterator		fIter;
};


class MDatabank : public MRSObject<MDatabank, struct MDatabankImp>
{
  public:
						MDatabank(const std::string& inName);

	static MDatabank*	Create(const std::string& inPath);
	static void			Merge(const std::string& inPath, MDatabankArray inDbs);

	long				Count();
	std::string			GetVersion();
	void				DumpInfo();
	
	long				CountForKey(const std::string& inIndex, const std::string& inKey) const;

	MQueryResults*		Find(const std::string& inQuery, bool inAutoWildcard = true);
	
	const char*			Get(const std::string& inEntryID);
	const char*			Sequence(const std::string& inEntryID, unsigned long inIndex = 0);
	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
	MIndex*				Index(const std::string& inIndex);
	MIndices*			Indices();
	MStringIterator*	SuggestCorrection(const std::string& inWord);
	
	// interface for creating new databanks
	
	void				Store(const std::string& inDocument);
	void				IndexText(const std::string& inIndex, const std::string& inText);
	void				IndexTextAndNumbers(const std::string& inIndex, const std::string& inText);
	void				IndexWord(const std::string& inIndex, const std::string& inText);
	void				IndexValue(const std::string& inIndex, const std::string& inText);
	void				IndexDate(const std::string& inIndex, const std::string& inText);
	void				IndexNumber(const std::string& inIndex, const std::string& inText);
	void				AddSequence(const std::string& inSequence);
	void				FlushDocument();

	void				SetVersion(const std::string& inVersion);

	void				Finish();

	// stupid swig...
	// now we have to pass the indices contatenated as a string, separated by colon
	void				CreateDictionary(std::string inIndices,
							long inMinOccurrence, long inMinWordLength);

  private:
						MDatabank(const std::string& inName, bool);
};

//class MQueryObject : public MRSObject<MQueryObject, struct MQueryObjectImp>
//{
//  public:
//	
//	static const std::string	kWildCardString;
//	
//	static MQueryObject*	Match(const std::string& inValue, const std::string& inIndex = kWildCardString);
//	static MQueryObject*	MatchRel(const std::string& inValue, const std::string& inRelOp, const std::string& inIndex = kWildCardString);
//	static MQueryObject*	Not(MQueryObject* inQuery);
//	static MQueryObject*	Union(MQueryObject* inQueryA, MQueryObject* inQueryB);
//	static MQueryObject*	Intersection(MQueryObject* inQueryA, MQueryObject* inQueryB);
//	
//	MQueryResults*			Find();
//};

class MQueryResults : public MRSObject<MQueryResults, struct MQueryResultsImp>
{
  public:
	const char*			Next();
	void				Skip(long inCount);
	
	unsigned long		Count(bool inExact) const;
	
	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
};

class MKeys : public MRSObject<MKeys, struct MKeysImp>
{
  public:
	const char*			Next();
	void				Skip(long inCount);
};

class MIndex : public MRSObject<MIndex, struct MIndexImp>
{
  public:

	std::string			Code() const;
	std::string			Type() const;
	long				Count() const;

	MKeys*				Keys();
	MKeys*				FindKey(const std::string& inKey);
};

class MIndices : public MRSObject<MIndices, struct MIndicesImp>
{
  public:
	MIndex*				Next();
};

class MBlastHit : public MRSObject<MBlastHit, struct MBlastHitImp>
{
  public:
	const char*			Id();
	
	MBlastHsps*			Hsps();
};

class MBlastHits : public MRSObject<MBlastHits, struct MBlastHitsImp>
{
  public:
	const char*			ReportInXML();
	MBlastHit*			Next();
};

class MBlastHsp : public MRSObject<MBlastHsp, struct MBlastHspImp>
{
  public:
	unsigned long		Score();
	double				BitScore();
	double				Expect();

	unsigned long		QueryStart();
	unsigned long		SubjectStart();
	
	std::string			QueryAlignment();
	std::string			SubjectAlignment();
};

class MBlastHsps : public MRSObject<MBlastHsps, struct MBlastHspsImp>
{
  public:
	MBlastHsp*			Next();
};


#endif
