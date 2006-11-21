/*
 * $Id$
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
 
#ifndef MRSINTERFACE_H
#define MRSINTERFACE_H

#include <string>
#include <vector>
#include "MObject.h"

class MDatabank;
typedef std::vector<MDatabank*> 	MDatabankArray;
typedef std::vector<std::string>	MStringArray;

std::string errstr();

#ifndef NO_BLAST
// a few BLAST helper functions

std::string DUST(const std::string& inSequence);
std::string SEG(const std::string& inSequence);
#endif

// some globals, settable from the outside
extern int VERBOSE;
extern unsigned int THREADS;
extern const char* COMPRESSION;
extern int COMPRESSION_LEVEL;
extern unsigned int WEIGHT_BIT_COUNT;
extern const char* COMPRESSION_DICTIONARY;
extern std::string gErrStr;

#ifndef NO_BLAST
class MBlastHits;
class MBlastHsps;
#endif
class MStringIterator;
class MIndices;
class MIndex;
class MBooleanQuery;
class MQueryResults;
class MRankedQuery;

struct MDatabankImp;
struct MBooleanQueryImp;
struct MQueryResultsImp;
struct MKeysImp;
struct MIndexImp;
struct MIndicesImp;
#ifndef NO_BLAST
struct MBlastHitImp;
struct MBlastHitsImp;
struct MBlastHspImp;
struct MBlastHspsImp;
#endif

/// MStringIterator, a helper class for access to lists of strings

//! Several routines in this MRS interface work with lists of strings.
//! The MStringIterator class can be used to iterate over the strings
//! in these lists.

class MStringIterator
{
  public:
	/** \brief MStringIterator constructor
	 *
	 *	The MStringIterator is constructed from an stl vector of strings
	 */

						MStringIterator(std::vector<std::string>& inStrings)
							: fStrings(inStrings)
							, fIter(fStrings.begin()) {}
	
	/** \brief Next string in list
	 *
	 *  Use Next to access the next element in the list. It will return
	 *	NULL if you've reached the end of the list.
	 */
	
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

/**	\brief MDatabank is the databank object
 *
 *	The MDatabank object represents a logical databank. This can either be a
 *	MRS file on disk or a 'Update' or 'Joined' databank.
 */

class MDatabank : public MRSObject<MDatabank, struct MDatabankImp>
{
  public:

	/**	\brief a constant string used as default parameter
	 */

	static const char	kWildCardString[];

#ifndef SWIG
						MDatabank();
#endif

	/**	\brief	Constructor used to open an existing databank
	 *	
	 *	The MDatabank constructor is used to open a databank.
	 *	\param inName the name of the databank to open. The name can be one of the following:
	 *			- a full path to an MRS file (e.g. <em>/usr/data/mrs/sprot.cmp</em> )
	 *			- a base name of an MRS file (e.g. \e sprot ) The file is looked up in the current
	 *				directory and if it cannot be found there the directory pointed to by the
	 *				MRS_DATA_DIR variable is used. The extension '.cmp' is added as well if needed.
	 *			- a joined databank name (e.g. <em>sprot+trembl</em> ) The two files are treated
	 *				as if they were one databank.
	 *			- an update databank (e.g. <em>embl_release|embl_updates</em> ) This construction
	 *				is used to filter the results from the first databank with the contents of
	 *				the second, effectively acting as if you have one databank to which an
	 *				update is applied.
	 *
	 *	Note that you can combine the join and update operators.
	 */

						MDatabank(const std::string& inName);

	/**	\brief	Create a new databank
	 *
	 *	The Create method is a static function that can be used to create a new databank.
	 *	A temporary file is created and the file is filled with Store* and Index* methods.
	 *	Once you're done with filling the file you call Finish.
	 *
	 *	\param inPath	The fully qualified path to the mrs file to create.
	 *	\param inMetaDataFields
	 * 					The list of meta data fields you want to create in this databank.
	 *	\return			The newly created databank.
	 *	\see			Finish
	 */

	static MDatabank*	Create(const std::string& inPath, MStringArray inMetaDataFields);
	static void			Merge(const std::string& inPath, MDatabankArray inDbs, bool inCopyData);

	long				Count();
	std::string			GetVersion();
	std::string			GetUUID();
	std::string			GetFilePath();
	bool				IsUpToDate();		// checks to see if our data still resides on disk
	long long			GetRawDataSize();
	void				DumpInfo();
	void				DumpIndex(const std::string& inIndex);
	
	void				PrefetchDocWeights(const std::string& inIndex);
	
	long				CountForKey(const std::string& inIndex, const std::string& inKey) const;

	MQueryResults*		Find(const std::string& inQuery, bool inAutoWildcard = true);

	MBooleanQuery*		Match(const std::string& inValue, const std::string& inIndex = kWildCardString);
	MBooleanQuery*		MatchRel(const std::string& inValue, const std::string& inRelOp, const std::string& inIndex = kWildCardString);

	MBooleanQuery*		BooleanQuery(const std::string& inQuery);
	MRankedQuery*		RankedQuery(const std::string& inIndex);
	
	const char*			Get(const std::string& inEntryID);
	const char*			GetMetaData(const std::string& inEntryID, const std::string& inFieldName);
	
						// by definition the description is the first meta data field.
	const char*			GetDescription(const std::string& inEntryID);

#ifndef NO_BLAST
	const char*			Sequence(const std::string& inEntryID, unsigned long inIndex = 0);
	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
#endif
	MIndex*				Index(const std::string& inIndex);
	MIndices*			Indices();
	MStringIterator*	SuggestCorrection(const std::string& inWord);
	
	// interface for creating new databanks
	
	void				SetStopWords(MStringArray inStopWords);
	
	void				StoreMetaData(const std::string& inFieldName, const std::string& inValue);
	void				Store(const std::string& inDocument);
	void				IndexText(const std::string& inIndex, const std::string& inText);
	void				IndexTextAndNumbers(const std::string& inIndex, const std::string& inText);
	void				IndexWord(const std::string& inIndex, const std::string& inText);
	void				IndexValue(const std::string& inIndex, const std::string& inText);
	void				IndexWordWithWeight(const std::string& inIndex,
							const std::string& inText, unsigned long inFrequency);
	void				IndexDate(const std::string& inIndex, const std::string& inText);
	void				IndexNumber(const std::string& inIndex, const std::string& inText);
#ifndef NO_BLAST
	void				AddSequence(const std::string& inSequence);
#endif
	void				FlushDocument();

	void				SetVersion(const std::string& inVersion);

	void				Finish(bool inCreateAllTextIndex = false);
//	void				RecalcDocWeights(const std::string& inIndex);

	// stupid swig...
	// now we have to pass the indices contatenated as a string, separated by colon
	void				CreateDictionary(std::string inIndices,
							long inMinOccurrence, long inMinWordLength);

  private:
						MDatabank(const std::string& inName, const MStringArray& inMetaDataFields);
};

class MBooleanQuery : public MRSObject<MBooleanQuery, struct MBooleanQueryImp>
{
	friend class MRankedQuery;
  public:
	
	static MBooleanQuery*	Not(MBooleanQuery* inQuery);
	static MBooleanQuery*	Union(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB);
	static MBooleanQuery*	Intersection(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB);
	
	MQueryResults*			Perform();

	void					Prefetch();
};

class MRankedQuery : public MRSObject<MRankedQuery, struct MRankedQueryImp>
{
  public:
	void				AddTerm(const std::string& inTerm, unsigned long inFrequency);

//	int					MaxReturn;
//	int					AllTermsRequired;

	void				SetAllTermsRequired(bool inRequired);
	void				SetMaxReturn(int inMaxReturn);
	void				SetAlgorithm(const std::string& inAlgorithm);

	MQueryResults*		Perform(MBooleanQuery* inMetaQuery = NULL);
};

class MQueryResults : public MRSObject<MQueryResults, struct MQueryResultsImp>
{
  public:
	const char*			Next();
	unsigned long		Score() const;

	void				Skip(long inCount);
	
	unsigned long		Count(bool inExact) const;
	
#ifndef NO_BLAST
	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
#endif
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
	
	float				GetIDF(const std::string& inKey);
};

class MIndices : public MRSObject<MIndices, struct MIndicesImp>
{
  public:
	MIndex*				Next();
};

#ifndef NO_BLAST
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

#endif
