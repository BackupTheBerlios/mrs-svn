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

	/**	\brief	Merge several databanks into one
	 *
	 *	Use the Merge static method to concatenate several databanks into one new file,
	 *	including the merge of the indices. Merged databanks are much more efficient than
	 *	joined databanks.
	 *
	 *	\param	inPath
	 *					The path for the merged databank to create.
	 *	\param	inDbs
	 *					An array of MDatabank objects
	 *	\param	inCopyData
	 *					A flag to indicate whether to copy the data or to simply put a link
	 *					statement in the merged file. If you link to the original files the
	 *					source databanks may not be deleted or you can no longer use the
	 *					merged databank. The UUID of the source databanks is recorded and
	 *					used to determine if they are still valid.
	 */

	static void			Merge(const std::string& inPath, MDatabankArray inDbs, bool inCopyData);

	long				Count();			/**< \brief Return the number of documents in this databank */
	std::string			GetVersion();		/**< \brief Return the version string for this databank */
	std::string			GetUUID();			/**< \brief Return the UUID for this databank */
	std::string			GetFilePath();		/**< \brief Return the path to the file for this databank */
	bool				IsUpToDate();		/**< \brief Checks to see if our data still resides on disk */
	long long			GetRawDataSize();	/**< \brief Return the original data size (size of all Stored documents) */
	void				DumpInfo();			/**< \brief Print to stdout some information about this databank */
	void				DumpIndex(const std::string& inIndex);
											/**< \brief Print on stdout the contents of index \a inIndex */
	
	/** \brief	Load the document weight vectors into memory for the specified index
	 *
	 *	In order to do ranked searches MRS needs a table with document weights for the index
	 *	used (usually this is \e __ALL_TEXT__ ). To speed up searching this table can be preloaded
	 *	into memory.
	 *
	 *	\param	inIndex
	 *					The index for which the weight table should be loaded (This should probably be \e __ALL_TEXT__ )
	 */
	
	void				PrefetchDocWeights(const std::string& inIndex);

	/** \brief	Count the documents for which inIndex contains inKey
	 *
	 *	\param	inIndex	The index to use
	 *	\param	inKey	The key to search for
	 */
	
	long				CountForKey(const std::string& inIndex, const std::string& inKey) const;

	/** \brief	Simple boolean search using a query in MRS search syntax
	 *
	 *	Use this Find method to do a simple and fast boolean search. The query specified is
	 *	parsed and split into search terms, optionally limited to an index with or without 
	 *	relational operators. See MRS user documentation for more information on syntax.
	 *
	 *	\param	inQuery	The query
	 *	\param	inAutoWildcard
	 *					Append a wildcard to each search term.
	 *	\return			A MQueryResults object that can be used directly to iterate over the results of the query.
	 */				

	MQueryResults*		Find(const std::string& inQuery, bool inAutoWildcard = true);

	/** \brief	Use Match to create a boolean query object
	 *
	 *	Boolean query objects are objects that represent a boolean query. They can be used in further construction
	 *	of more complex boolean queries. Match returns a simple boolean query object that does exact matching of
	 *	\a inValue in \a inIndex.
	 *
	 *	\param	inValue	The value to search for.
	 *	\param	inIndex	The index to search, the default is to search all indices available.
	 *	\return	The MBooleanQuery object
	 */

	MBooleanQuery*		Match(const std::string& inValue, const std::string& inIndex = kWildCardString);

	/** \brief	Use MatchRel to create a boolean query object with some relational operator
	 *
	 *	Boolean query objects are objects that represent a boolean query. They can be used in further construction
	 *	of more complex boolean queries. MatchRel is used to create a more complex boolean query object than Match.
	 *	You can specify relational operators in string representation:
	 *		- ':'	The document should contain the value inValue in its index inIndex. (for full text search, the default)
	 *		- '='	The value should be exactly the same
	 *		- '<'	The value should be less than \a inValue
	 *		- '<='	The value should be less than or equal to \a inValue
	 *		- '>='	The value should be greater than or equal to \a inValue
	 *		- '<'	The value should be greater than \a inValue
	 *
	 *	\param	inValue	The value to search for.
	 *	\param	inRelOp	The relational operator to use.
	 *	\param	inIndex	The index to search, the default is to search all indices available.
	 *	\return	The MBooleanQuery object
	 */

	MBooleanQuery*		MatchRel(const std::string& inValue, const std::string& inRelOp, const std::string& inIndex = kWildCardString);

	/** \brief	An alternative to the Match methods, use inQuery instead
	 *
	 *	To create a boolean query object you can also use BooleanQuery, it acts the same as Find but returns
	 *	a MBooleanQuery object instead of a MQueryResults object.
	 *	\param	inQuery	A query in MRS query syntax.
	 *	\result			The MBooleanQuery object
	 */

	MBooleanQuery*		BooleanQuery(const std::string& inQuery);

	/** \brief	Create an empty MRankedQuery object
	 *
	 *	To start a ranked search you first have to create a MRankedQuery object and fill it later.
	 *	\param	inIndex	The weighted index to search.
	 *	\result			The MRankedQuery object
	 */

	MRankedQuery*		RankedQuery(const std::string& inIndex);

	/** \brief	Return a document by ID
	 *
	 *	This method returns the stored document by \a inEntryID.
	 *	\param	inEntryID
	 *					The documents ID.
	 *	\result			The text of the document or NULL if it was not found.
	 */
	
	const char*			Get(const std::string& inEntryID);

	/** \brief	Return the content of a meta data field for a document specified by ID
	 *
	 *	This method returns the content of the meta data field \a inFieldName for the document specified by \a inEntryID.
	 *	\param	inEntryID
	 *					The document ID.
	 *	\param	inFieldName
	 *					The meta data field name.
	 *	\result			The text of the meta data field or NULL if it was not found.
	 */
	
	const char*			GetMetaData(const std::string& inEntryID, const std::string& inFieldName);
	
	/** \brief	Return the description of a document
	 *
	 *	By definition the first meta data field contains the description (or title if you prefer)
	 *	for a document. This method returns the content of that first meta data field for the 
	 *	document specified by \a inEntryID.
	 *	\param	inEntryID
	 *					The documents ID.
	 *	\result			The text of the meta data field or NULL if it was not found.
	 */
	
	const char*			GetDescription(const std::string& inEntryID);

#ifndef NO_BLAST

	/** \brief	Return sequence \a inIndex stored for document \a inEntryID
	 *
	 *	Protein sequences can be stored separately to make it possible to search
	 *	them using the BLAST algorithm. This method retrieves a sequence by \a inEntryID
	 *	and \a inIndex. The index is required for databanks like PDB where you can have
	 *	more than one sequence per document.
	 *	\param	inEntryID
	 *					The documents ID.
	 *	\param	inIndex
	 					The index number for the sequence to return. The default is 0
	 					which corresponds with the first sequence. Numbering is 0 based.
	 *	\result			The sequence or NULL if it was not found.
	 */

	const char*			Sequence(const std::string& inEntryID, unsigned long inIndex = 0);

	/** \brief	Perform a blast search
	 *
	 *	MRS can do searches using the BLAST algorithm. Only basic parameters are supported
	 *	in this version of MRS and only blastp is implemented.
	 *	\param	inQuery	The query sequence
	 *	\param	inMatrix
	 *					The matrix to use. The default is the built-in BLOSUM62 matrix. Other
	 *					matrices can be specified but then you must have the matrix file available
	 *					in the current directory. (This needs to be fixed someday)
	 *	\param	inWordSize
	 *					The wordsize to use.
	 *	\param	inExpect
	 *					The minimal e-value.
	 *	\param	inFilter
	 *					Whether to use low complexity filtering or not.
	 *	\param	inGapped
	 *					Whether to allow gaps in the search result or not.
	 *	\param	inGapOpen
	 *					The cost for opening a gap
	 *	\param	inGapExtend
	 *					The cost for extending a gap
	 *	\return			The MBlastHits object that can be iterated for blast results
	 */

	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
#endif

	MIndex*				Index(const std::string& inIndex);	/**< \brief Return an MIndex object for index \a inIndex */
	MIndices*			Indices();							/**< \brief Return an MIndices object to iterate over all indices */

	/** \brief	Spellcheck a word
	 *
	 *	MRS can do spellchecking based on the contents of the indices. If a dictionary file is
	 *	available, this method will return a list of suggested spelling variants found in the databank.
	 *	\param	inWord	The word to spell check
	 *	\return			The MStringIterator object that can be used to iterate over all results
	 */

	MStringIterator*	SuggestCorrection(const std::string& inWord);
	
	// interface for creating new databanks

	/** \brief	Set the list of stopwords to use for the __ALL_TEXT__ index
	 *
	 *	When constructing the special __ALL_TEXT__ index a list of stop words is
	 *	consulted to avoid entry of too common terms that can skew searching results.
	 *	\param	inStopWords
	 *					The list of stop words to use.
	 */
	
	void				SetStopWords(MStringArray inStopWords);

	/** \brief	Store a value for a meta data field
	 *
	 *	MRS can store values in a meta data field along with the document itself.
	 *	This can be used e.g. to avoid having to parse the documents in order to get a title.
	 *	\param	inFieldName
	 *					The name of the meta data field to set. This name must also have been
	 *					specified when you created this databank using Create.
	 *	\param	inValue	
	 *					The value to set.
	 */
	
	void				StoreMetaData(const std::string& inFieldName, const std::string& inValue);

	/** \brief	Store a document
	 *
	 *	Set the contents of the document currently being inserted. You commit this by calling
	 *	FlushDocument.
	 *	\param	inDocument	
	 *					The text for the document
	 */
	
	void				Store(const std::string& inDocument);

	/** \brief	Store the words in inText into index inIndex
	 *
	 *	This version of the Index* calls tokenizes \a inText and stores the words into
	 *	full text index \a inIndex. If \a inIndex is not known yet, it is created.
	 *	Note that this version does not index numbers.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The text to tokenize and index.
	 */

	void				IndexText(const std::string& inIndex, const std::string& inText);

	/** \brief	Store the words and numbers in inText into index inIndex
	 *
	 *	This version of the Index* calls tokenizes \a inText and stores the words into
	 *	full text index \a inIndex. If \a inIndex is not known yet, it is created.
	 *	Note that this version does index numbers, use it instead of IndexText if you need to index numbers.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The text to tokenize and index.
	 */

	void				IndexTextAndNumbers(const std::string& inIndex, const std::string& inText);

	/** \brief	Store a single word into full text index inIndex
	 *
	 *	This version of the Index* calls stores the exact word \a inText into the full text index \a inIndex.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The word to index.
	 */

	void				IndexWord(const std::string& inIndex, const std::string& inText);

	/** \brief	Store a single value into value index inIndex
	 *
	 *	This version of the Index* calls is used to create a unique index, i.e. one word
	 *	always corresponds to exactly one document. The special value index 'id' is used
	 *	later on for document ID's.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The value to index.
	 */

	void				IndexValue(const std::string& inIndex, const std::string& inText);

	/** \brief	Store a single word into weighted index inIndex
	 *
	 *	This version of the Index* calls stores the exact word \a inText together
	 *	with the inFrequency value into weighted index \a inIndex.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The word to index.
	 *	\param	inFrequency
	 *					The frequency for the \a inText word. This frequency is
	 *					normalised at the time you call FlushDocument.
	 */

	void				IndexWordWithWeight(const std::string& inIndex,
							const std::string& inText, unsigned long inFrequency);

	/** \brief	Store a date into date index inIndex
	 *
	 *	This version of the Index* calls stores the date \a inText into index \a inIndex.
	 *	Date should be formatted exactly as 'YYYY-MM-DD'.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The date to index.
	 */

	void				IndexDate(const std::string& inIndex, const std::string& inText);

	/** \brief	Store a number into number index inIndex
	 *
	 *	This version of the Index* calls stores the number \a inText into index \a inIndex.
	 *	Only integral values are allowed for now.
	 *	\param	inIndex	The index to use (and create if needed).
	 *	\param	inText	The number to index.
	 */

	void				IndexNumber(const std::string& inIndex, const std::string& inText);

#ifndef NO_BLAST

	/** \brief	Store a sequence along with a document
	 *
	 *	Use this method to store sequences along with documents to be used later on
	 *	using BLAST searches.
	 *	\param	inSequence
	 *					The sequence to store
	 */

	void				AddSequence(const std::string& inSequence);
#endif

	/** \brief	Tell MRS we're done with the document
	 *
	 *	After Storing our document and sequences and calling the various Index* methods
	 *	we tell MRS it can flush these changes and it should reset everything so we
	 *	can either start a new document or Finish the databank.
	 */

	void				FlushDocument();

	/** \brief	Store a version string along with the databank
	 *
	 *	Insert the version information for the raw data from which this databank was constructed.
	 *	\param	inVersion
	 *					The text for the version string
	 */

	void				SetVersion(const std::string& inVersion);

	/** \brief	Create the final MRS file.
	 *
	 *	When all documents have been processed you can ask MRS to create the final .cmp file
	 *	and construct all indices. The parameter \a inCreateAllTextIndex is used to tell MRS
	 *	to create a special __ALL_TEXT__ index that contains all words passed to the Index* methods.
	 *	This __ALL_TEXT__ index is a weighted index and can be used to do ranked searches.
	 */

	void				Finish(bool inCreateAllTextIndex = false);

	/** \brief	Create a dictionary file for indices \a inIndices
	 *
	 *	In order to use the SuggestCorrection method you have to create a .dict file
	 *	first. You can do so using this method.
	 *	\param	inIndices
	 *					a colon separated list of index names to use for the construction
	 *					of the dictionary. You should probably use __ALL_TEXT__ for this.
	 *	\param	inMinOccurrence
	 *					The minimal number of documents that should contain the words
	 *					in the dictionary. To avoid spurious typing errors in the dict file.
	 *	\param	inMinWordLength
	 *					The minimal word length for entries in the dict file. 4 is maybe a good value.
	 */

	void				CreateDictionary(std::string inIndices,
							long inMinOccurrence, long inMinWordLength);

  private:
						MDatabank(const std::string& inName, const MStringArray& inMetaDataFields);
};

/** \brief	An object that represents a boolean query
 *
 *	Use the MBooleanQuery object to construct complex boolean queries on the fly.
 */

class MBooleanQuery : public MRSObject<MBooleanQuery, struct MBooleanQueryImp>
{
	friend class MRankedQuery;
  public:

	static MBooleanQuery*	Not(MBooleanQuery* inQuery);
								/**< \brief	Create a new MBooleanQuery object that negates the results of \a inQuery */

	static MBooleanQuery*	Union(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB);
								/**< \brief	Create the union of \a inQueryA and \a inQueryB */

	static MBooleanQuery*	Intersection(MBooleanQuery* inQueryA, MBooleanQuery* inQueryB);
								/**< \brief	Create the intersection of \a inQueryA and \a inQueryB */

	/** \brief	Perform the query
	 *
	 *	To actually perform a boolean search you have to call Perform. This will return an
	 *	iterator object that you can use to access the results. Perform can be called more than
	 *	once.
	 *	\return				The MQueryResults object that can be used to iterate over the results. This
	 *						value is NULL if nothing was found.
	 */

	MQueryResults*			Perform();

	/** \brief	Perform the query and store the results internally
	 *
	 *	If you know you will use this boolean query object more often you might want to call Prefetch
	 *	to avoid having to do the actual search more than once.
	 */

	void					Prefetch();
};

/** \brief	An object that represents a ranked query
 *
 *	Use the MRankedQuery object to do ranked searches
 */

class MRankedQuery : public MRSObject<MRankedQuery, struct MRankedQueryImp>
{
  public:
	/** \brief	Add a term to the query object
	 *
	 *	A ranked query search is a initially empty. Use AddTerm to store terms and specify
	 *	the importance of these terms by using the inFrequency parameter.
	 *	\param	inTerm	The term to include in this search
	 *	\param	inFrequency
	 *					The frequency for this term
	 */

	void				AddTerm(const std::string& inTerm, unsigned long inFrequency);

	/** \brief	Add terms from a string of text
	 *
	 *	This method tokenizes the \a inText parameter in the same way as the IndexText
	 *	method of MDatabank. It uses the resulting tokens (words) to construct a new
	 *	MRankedQuery object. Useful to do a <em>FindSimilar</em> kind of search.
	 *	\param	inTerm	The term to include in this search
	 *	\param	inFrequency
	 *					The frequency for this term
	 */

	void				AddTermsFromText(const std::string& inText);

//	I wish swig worked a bit better...
//	int					MaxReturn;
//	int					AllTermsRequired;

	/** \brief	Specify whether all terms should be found
	 *
	 *	If you set 'all terms required' to false, the query might return
	 *	documents in which some of the terms are absent.
	 */
	void				SetAllTermsRequired(bool inRequired);

	/** \brief	Specify how many results to return at maximum
	 *
	 *	Ranked queries have to search all documents and construct a list
	 *	of based on that. This parameter influences the maximum number of
	 *	hits to return.
	 *	\param	inMaxReturn	The maximum number of hits to return.
	 */
	void				SetMaxReturn(int inMaxReturn);

	/** \brief	Set the algorithm to use
	 *
	 *	Several algorithms exist for doing ranked searches. See a textbook on
	 *	Information Retrieval to learn what these algorithms do.
	 *		- \e vector 	The classic vector algorithm using cosine similarity
	 *		- \e dice		An alternative to vector
	 *		- \e jaccard	Another alternative to vector
	 *	\param	inAlgorithm	The algorithm to use.
	 */
	void				SetAlgorithm(const std::string& inAlgorithm);
	
	/** \brief	Perform the actual search
	 *
	 *	To actually perform the search you call Perform. The parameter \a inMetaQuery
	 *	is used as a filter for the results. It is a boolean query and only documents
	 *	that are found in this query are candidates for scoring.
	 *	\param	inMetaQuery	The filtering boolean query to use. Pass NULL if you don't
	 *						want to use a filter.
	 *	\result			The MQueryResults object or NULL if nothing was found.
	 */

	MQueryResults*		Perform(MBooleanQuery* inMetaQuery = NULL);
};

/** \brief	An iterator object to iterate over query results
 *
 *	All query objects return the same MQueryResults object. You use this
 *	object to iterate over the results of the performed query.
 */

class MQueryResults : public MRSObject<MQueryResults, struct MQueryResultsImp>
{
  public:

	/** \brief	Return the next document ID or NULL if there is none.
	 *
	 *	The Next method returns the next document ID or NULL if you've hit the end
	 *	of the result list.
	 */

	const char*			Next();

	/** \brief	Return the score for the document returned by the last Next call
	 *
	 *	This method is of course only useful for ranked searches. It returns the
	 *	score the document of which the ID was retrieved using the last Next call.
	 *	The range for Score is 0 < score <= 100.
	 */

	unsigned long		Score() const;

	/**	\brief	Skip over a number of results
	 *
	 *	To support paging through results you can use the Skip method to skip
	 *	over an initial number of results.
	 */

	void				Skip(long inCount);
	
	/**	\brief	Return the number of hits in this MQueryResults object
	 *
	 *	The number of hits in this MQueryResults object can be exact or guessed.
	 *	\param inExact	Tell MRS to calculate the number of hits exactly or to return a guess.
	 */

	unsigned long		Count(bool inExact) const;
	
#ifndef NO_BLAST
	/** \brief	Perform a BLAST search using the result of a query as databank
	 *
	 *	To blast against the results of a query you can use this method. It is an alternative
	 *	to MDatabank::Blast
	 *	MRS can do searches using the BLAST algorithm. Only basic parameters are supported
	 *	in this version of MRS and only blastp is implemented.
	 *	\param	inQuery	The query sequence
	 *	\param	inMatrix
	 *					The matrix to use. The default is the built-in BLOSUM62 matrix. Other
	 *					matrices can be specified but then you must have the matrix file available
	 *					in the current directory. (This needs to be fixed someday)
	 *	\param	inWordSize
	 *					The wordsize to use.
	 *	\param	inExpect
	 *					The minimal e-value.
	 *	\param	inFilter
	 *					Whether to use low complexity filtering or not.
	 *	\param	inGapped
	 *					Whether to allow gaps in the search result or not.
	 *	\param	inGapOpen
	 *					The cost for opening a gap
	 *	\param	inGapExtend
	 *					The cost for extending a gap
	 *	\return			The MBlastHits object that can be iterated for blast results
	 */

	MBlastHits*			Blast(const std::string& inQuery, const std::string& inMatrix,
							unsigned long inWordSize, double inExpect, bool inFilter,
							bool inGapped, unsigned long inGapOpen, unsigned long inGapExtend);
#endif
};

/** \brief	An iterator for keys in an index */

class MKeys : public MRSObject<MKeys, struct MKeysImp>
{
  public:
	const char*			Next();					/**< Return next key or NULL when none is left */
	void				Skip(long inCount);		/**< Skip over \a inCount keys */
};

/** \brief	An object containing information about an index */

class MIndex : public MRSObject<MIndex, struct MIndexImp>
{
  public:

	std::string			Code() const;			/**< Return the id for this index */
	std::string			Type() const;			/**< Return the type for this index */
	long				Count() const;			/**< Return the number of entries in this index */

	MKeys*				Keys();					/**< Return an iterator object for the keys in this index */
	MKeys*				FindKey(const std::string& inKey);
												/**< Return an iterator object for the keys in this index starting at \a inKey */
	
	float				GetIDF(const std::string& inKey);
												/**< Return the Inverse Document Frequency for a inKey, used in ranked searches */
};

/** \brief An object that iterates over all indices in a databank */

class MIndices : public MRSObject<MIndices, struct MIndicesImp>
{
  public:
	MIndex*				Next();					/**< Return the next MIndex or NULL when none is left */
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
