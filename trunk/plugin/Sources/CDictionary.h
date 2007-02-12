/*	$Id: CDictionary.h 185 2006-11-21 09:35:23Z hekkel $
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

	CDatabankBase&						fDatabank;
	std::auto_ptr<HFileStream>			fDictionaryFile;
	std::auto_ptr<HMMappedFileStream>	fMemMapper;
	const union CTransition*			fAutomaton;
	uint32								fAutomatonLength;

	struct CScoreTable*					fScores;
	std::string							fWord;
};


#endif // CDICTIONARY_H
