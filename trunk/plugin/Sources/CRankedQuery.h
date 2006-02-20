/*	$Id$
	Copyright hekkelman
	Created Monday February 20 2006 14:00:30
*/

#ifndef CRANKEDQUERY_H
#define CRANKEDQUERY_H

#include "HStlString.h"

class CDocIterator;
class CDatabankBase;

class CRankedQuery
{
  public:
					CRankedQuery();
	virtual			~CRankedQuery();
	
	void			AddTerm(const std::string& inKey, float inWeight);
	
	CDocIterator*	PerformSearch(CDatabankBase& inDatabank,
						const std::string& inIndex);
	
  private:
					CRankedQuery(const CRankedQuery&);
	CRankedQuery&	operator=(const CRankedQuery&);

	struct CRankedQueryImp*		fImpl;
};

#endif // CRANKEDQUERY_H
