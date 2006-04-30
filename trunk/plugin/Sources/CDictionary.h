/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Thursday August 25 2005 12:46:55
*/

#ifndef CDICTIONARY_H
#define CDICTIONARY_H

#include <string>
#include <vector>

#include "HStream.h"

class CDatabankBase;

class CDictionary
{
  public:
	static void		Create(CDatabankBase& inDatabank,
						const std::vector<std::string>& inIndexNames,
						uint32 inMinOccurrence, uint32 inMinWordLength);

					CDictionary(CDatabankBase& inDatabank);
					
					~CDictionary();

	std::vector<std::string>
					SuggestCorrection(const std::string& inWord);

  private:

	void			Test(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	bool			Match(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	void			Delete(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	void			Insert(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	void			Transpose(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	void			Substitute(uint32 inState, int32 inScore, uint32 inEdits,
						std::string inMatch, const char* inWoord);
	void			AddSuggestion(int32 inScore, uint32 inEdits, std::string inMatch);

	typedef std::vector<std::pair<std::string,int32> > CScores;
	
	struct CSortOnWord
	{
		bool operator()(const std::pair<std::string,int32>& inA, const std::pair<std::string,int32>& inB)
			{ return inA.first < inB.first; }
	};
	
	struct CSortOnScore
	{
		bool operator()(const std::pair<std::string,int32>& inA, const std::pair<std::string,int32>& inB)
			{ return inA.second > inB.second; }
	};

	CDatabankBase&						fDatabank;
	std::auto_ptr<HFileStream>			fDictionaryFile;
	std::auto_ptr<HMMappedFileStream>	fMemMapper;
	const union CTransition*			fAutomaton;
	uint32								fAutomatonLength;

	CScores								fHits;
	int32								fCutOff;
	std::string							fWord;
};


#endif // CDICTIONARY_H
