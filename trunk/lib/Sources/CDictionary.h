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


	CDatabankBase&						fDatabank;
	std::auto_ptr<HFileStream>			fDictionaryFile;
	std::auto_ptr<HMMappedFileStream>	fMemMapper;
	const union CTransition*			fAutomaton;
	uint32								fAutomatonLength;
};


#endif // CDICTIONARY_H
