/*	$Id: CQuery.h 331 2007-02-12 07:44:10Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Tuesday January 07 2003 20:12:37
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

#ifndef CQUERY_H
#define CQUERY_H

#include <string>
#include <vector>
#include <limits>

#include <boost/shared_ptr.hpp>

class CDatabankBase;
class CDocIterator;

class CQueryObject
{
  public:
							CQueryObject(CDatabankBase& inDb)
								: fDb(inDb) {}
	virtual					~CQueryObject() {}
	
	CDocIterator*			Perform();

	void					Prefetch();
	
  protected:

	virtual CDocIterator*	DoPerform() = 0;

	CDatabankBase&			fDb;
	std::vector<uint32>		fPrefetch;
};

class CMatchQueryObject : public CQueryObject
{
  public:
							CMatchQueryObject(CDatabankBase& inDb,
								const std::string& inValue,
								const std::string& inIndex);
							CMatchQueryObject(CDatabankBase& inDb,
								const std::string& inValue,
								const std::string& inRelOp,
								const std::string& inIndex);

  protected:

	virtual CDocIterator*	DoPerform();

	std::string				fValue;
	std::string				fIndex;
	CQueryOperator			fRelOp;
	bool					fIsPattern;
};

class CNotQueryObject : public CQueryObject
{
  public:
							CNotQueryObject(CDatabankBase& inDb,
								boost::shared_ptr<CQueryObject> inChild);

  private:

	virtual CDocIterator*	DoPerform();

	boost::shared_ptr<CQueryObject>		fChild;
};

class CUnionQueryObject : public CQueryObject
{
  public:
							CUnionQueryObject(CDatabankBase& inDb,
								boost::shared_ptr<CQueryObject> inObjectA,
								boost::shared_ptr<CQueryObject> inObjectB);

  private:

	virtual CDocIterator*	DoPerform();

	boost::shared_ptr<CQueryObject>		fChildA;
	boost::shared_ptr<CQueryObject>		fChildB;
};

class CIntersectionQueryObject : public CQueryObject
{
  public:
							CIntersectionQueryObject(CDatabankBase& inDb,
								boost::shared_ptr<CQueryObject> inObjectA,
								boost::shared_ptr<CQueryObject> inObjectB);

  private:

	virtual CDocIterator*	DoPerform();

	boost::shared_ptr<CQueryObject>		fChildA;
	boost::shared_ptr<CQueryObject>		fChildB;
};

class CParsedQueryObject : public CQueryObject
{
  public:
							CParsedQueryObject(CDatabankBase& inDb,
								const std::string& inQuery, bool inAutoWildcard);

  private:

	virtual CDocIterator*	DoPerform();

	std::string				fQuery;
	bool					fAutoWildcard;
};

#endif // CQUERY_H
