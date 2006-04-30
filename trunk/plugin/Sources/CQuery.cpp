/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Tuesday January 07 2003 20:13:07
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
 
#include "MRS.h"

#include <sstream>
#include <iostream>
#include "HError.h"
#include "HUtils.h"

#include "CQuery.h"
#include "CDatabank.h"
#include "CBitStream.h"
#include "HStream.h"
#include "CIndexer.h"
#include "CTokenizer.h"
#include "CIndexPage.h"

using namespace std;

enum CQueryToken
{
	qeUndefined	=	-1,
	qeEnd		=	256,
	qeString,
	qeNumber,
	qeIdent,
	qePattern,
	
	qeAND,
	qeOR,
	qeNOT,
	
	qeLT,
	qeLE,
	qeEQ,
	qeGE,
	qeGT
};

struct CQueryImp
{
  public:
							CQueryImp(
								CDatabankBase&	inDatabank,
								const string&	inQuery,
								bool 			inAutoWildcard);
							CQueryImp(
								CDatabankBase&	inDatabank);
							~CQueryImp();

	CDocIterator*			Build(const string& inIndex, const string& inValue,
								CQueryOperator inRelOp);

	CDocIterator*			Parse();
	
	CDatabankBase&			GetDatabank() const		{ return fDatabank; }

  private:
	
	void					GetIndexNames();

	// tokenizer
	
	void					Match(CQueryToken inToken);
	void					Restart(int& ioStart, int& ioState);
	void					Retract();
	CQueryToken				Unescape(string &ioToken);
	char					GetNextChar();
	CQueryToken				GetNextToken();
	
	// parser
	CDocIterator*			Parse_Query();
	CDocIterator*			Parse_Test();
	CDocIterator*			Parse_QualifiedTest(const string& inIndex);
	CDocIterator*			Parse_Term(const string& inIndex);
	CDocIterator*			IteratorForTerm(const string& inIndex,
								const string& inTerm, bool inTermIsPattern);
	CDocIterator*			IteratorForOperator(const string& inIndex,
								const string& inKey, CQueryOperator inOperator);
	CDocIterator*			IteratorForString(const string& inIndex, const string& inString);

	string					DescribeToken(int inToken);
	
	string					fQuery;
	CDatabankBase&			fDatabank;
	int64					fDataOffset;

	uint32					fOffset;
	uint32					fTokenOffset;
	string					fToken;
	double					fTokenValue;
	CQueryToken				fLookahead;
	vector<string>			fIndexNames;
	HStreamBase*			fData;
	vector<uint32>			fDocs;
	bool					fAutoWildcard;
};

CQueryImp::CQueryImp(CDatabankBase& inDatabank, const string& inQuery,
		bool inAutoWildcard)
	: fQuery(inQuery)
	, fDatabank(inDatabank)
	, fOffset(0)
	, fTokenOffset(0)
	, fAutoWildcard(inAutoWildcard)
{
	GetIndexNames();
}

CQueryImp::CQueryImp(CDatabankBase& inDatabank)
	: fDatabank(inDatabank)
{
	GetIndexNames();
}

CQueryImp::~CQueryImp()
{
}

void CQueryImp::GetIndexNames()
{
	for (uint32 n = 0; n < fDatabank.GetIndexCount(); ++n)
	{
		string code, type;
		uint32 count;
		
		fDatabank.GetIndexInfo(n, code, type, count);
		fIndexNames.push_back(code);
	}
}

CDocIterator* CQueryImp::Build(const string& inIndex,
	const string& inValue, CQueryOperator inRelOp)
{
	CDocIterator* result;

	if (inIndex == "*")
		result = IteratorForTerm(inIndex, inValue, inValue.find('*') != string::npos or inValue.find('?') != string::npos);
	else
		result = IteratorForOperator(inIndex, inValue, inRelOp);
	
	return result;
}

CDocIterator* CQueryImp::Parse()
{
	auto_ptr<CDocIterator> result;

	// shortcut
	if (fQuery == "*")
		result.reset(new CDbAllDocIterator(fDatabank.Count()));
	else
	{
		fLookahead = GetNextToken();
		result.reset(Parse_Query());
		
		if (result.get() == nil)
			THROW(("Failed to parse query"));
		
		// here we fetch the first 1000 or so doc Id's
		// for this query.
		// this is to limit extreme slow searches

#if P_DEBUG
		result->PrintHierarchy(0);
#endif

		result.reset(new CDocCachedIterator(result.release(), 1000));
	}

	return result.release();
}

template<class T>
basic_ostream<T>& operator<<(basic_ostream<T>& os, CQueryToken inToken)
{
	switch (inToken)
	{
		case qeEnd:					os << "end of query"; 	break;
		case qeString:				os << "string";			break;
		case qeNumber:				os << "number"; 		break;
		case qeIdent:				os << "identifier"; 	break;
		case qePattern:				os << "pattern";		break;
		case qeAND:					os << "AND";			break;
		case qeOR:					os << "OR";				break;
		case qeNOT:					os << "NOT";			break;
		case qeLT:					os << "'<='";			break;
		case qeLE:					os << "'<'";			break;
		case qeEQ:					os << "'='";			break;
		case qeGE:					os << "'>='";			break;
		case qeGT:					os << "'>='";			break;

		default:
		{
			if (inToken == '\'')
				os << '"' << char(inToken) << '"';
			else if (isprint(inToken))
				os << "'" << char(inToken) << "'";
			else
				os << "token(0x" << ios::hex << int(inToken) << ")";
			break;
		}
	}
	
	return os;
}

void CQueryImp::Match(CQueryToken inToken)
{
	if (fLookahead != inToken)
	{
		stringstream s;
		s << "Parse error: expected " << inToken << " but found " << fLookahead;
		THROW((s.str().c_str()));
	}
	fLookahead = GetNextToken();
}

char CQueryImp::GetNextChar()
{
	char result = 0;
	
	if (fOffset < fQuery.length())
		result = fQuery[fOffset];

	++fOffset;
	fToken += result;

	return result;
}

void CQueryImp::Restart(int& ioStart, int& ioState)
{
	switch (ioStart)
	{
		case 0:
			ioStart = 10;
			break;
		
		case 10:
//			ioStart = 20;
//			break;
//		
//		case 20:
			ioStart = 30;
			break;
		
		case 30:
			ioStart = 40;
			break;
	}
	
	ioState = ioStart;
	
	fToken.clear();
	fOffset = fTokenOffset;
}

void CQueryImp::Retract()
{
	--fOffset;

	if (fToken.length() > 0)
		fToken.erase(fToken.length() - 1, 1);
}

/*
 * Unescape a string, and determine if there are any unescaped
 * pattern characters in it.
 * If so, return qePattern, otherwise qeIdent.
 */
CQueryToken CQueryImp::Unescape(string &ioToken)
{
	CQueryToken token = qeIdent;
	for (string::iterator c = ioToken.begin(); c != ioToken.end(); ++c)
	{
		if (*c == '\\')
		{
			string::iterator n = c + 1;
			if (n == ioToken.end())
				break;
			
			switch (*n)
			{
				case 'r':
					*c = '\r';
					fToken.erase(n);
					break;
				
				case 'n':
					*c = '\n';
					fToken.erase(n);
					break;
				
				case 't':
					*c = '\t';
					fToken.erase(n);
					break;
				
				default:
					// erase the \ ; the ++c will skip over the next char
					// so it will not be tampered with later.
					fToken.erase(c);
					break;
			}
		}
		else if (*c == '*' or *c == '?')
		{
			token = qePattern;
		}
	}

	return token;
}

CQueryToken CQueryImp::GetNextToken()
{
	int state = 0, start = 0;
	bool escape = false, escaped;

	CQueryToken token = qeUndefined;
	fToken.clear();
	fTokenOffset = fOffset;
	
	while (token == qeUndefined)
	{
		char ch = GetNextChar();
		
		switch (state)
		{
			// first block, white space
			
			case 0:
				if (ch == 0)
					token = qeEnd;
				else if (not isspace(ch))
					Restart(start, state);
				else
					fTokenOffset = fOffset;
				break;
			
			// second block, quoted strings
			
			case 10:
				if (ch == '"')
				{
					fToken.clear();
					state = 11;
				}
				else
					Restart(start, state);
				break;
			
			case 11:
				if (ch == 0)
					THROW(("Parse error: unterminated string"));
				else if (ch == '"' and not escape)
				{
					fToken.erase(fToken.length() - 1, 1);
					token = qeString;
					Unescape(fToken);
				}
				else
					escape = not escape and ch == '\\';
				break;
			
			// block three, numbers
			
			case 20:
				if (isdigit(ch))
					state = 21;
				else
					Restart(start, state);	// not implemented yet
				break;
			
			case 21:
				if (ch == '.')
					Restart(start, state);
				else if (not isdigit(ch))
				{
					Retract();
					token = qeNumber;
					fTokenValue = strtod(fToken.c_str(), NULL);
				}
				break;
			
			// block four, identifiers
			
			case 30:
				if (ch == '\\')
					escape = true;
				else if (escape or isalnum(ch) or ch == '_' or ch == '*' or ch == '?' or ch == '.' or ch == '-')
					state = 31;
				else
					Restart(start, state);
				break;
			
			case 31:
				escaped = escape;
				escape = not escape and ch == '\\';
				if (not (escape or escaped or isalnum(ch) or ch == '_' or ch == '*' or ch == '?' or ch == '.' or ch == '-'))
				{
					Retract();
					token = Unescape(fToken);
				}
				break;
			
			// block five, rest
			
			case 40:
				switch (ch)
				{
					case '|':
						token = qeOR;
						break;
					
					case '&':
						token = qeAND;
						break;
					
					case '-':
						token = qeNOT;
						break;
					
					case '<':
						if (GetNextChar() == '=')
							token = qeLE;
						else
						{
							Retract();
							token = qeLT;
						}
						break;
					
					case '>':
						if (GetNextChar() == '=')
							token = qeGE;
						else
						{
							Retract();
							token = qeGT;
						}
						break;
					
					case '=':
						token = qeEQ;
						break;
					
					default:
						token = CQueryToken(ch);
						break;
				}
				break;
		}			
	}
	
	if (token == qeIdent)
	{
		if (fToken == "OR")
			token = qeOR;
		else if (fToken == "AND")
			token = qeAND;
		else if (fToken == "NOT")
			token = qeNOT;
	}
	
	return token;
}

CDocIterator* CQueryImp::Parse_Query()
{
	auto_ptr<CDocIterator> result(Parse_Test());

	while (fLookahead != qeEnd and fLookahead != ')')
	{
		switch (fLookahead)
		{
			case qeOR:
				Match(qeOR);
				result.reset(CDocUnionIterator::Create(result.release(), Parse_Test()));
				break;
			
			case qeAND:
				Match(qeAND);
				// fall through
			default:
				result.reset(CDocIntersectionIterator::Create(result.release(), Parse_Test()));
				break;
		}
	}
	
	return result.release();
}

CDocIterator* CQueryImp::Parse_Test()
{
	auto_ptr<CDocIterator> result(nil);
	
	switch (fLookahead)
	{
		case '(':
			Match(CQueryToken('('));
			result.reset(Parse_Query());
			Match(CQueryToken(')'));
			break;
		
		case qeNOT:
			Match(qeNOT);
			result.reset(new CDocNotIterator(Parse_Test(), fDatabank.Count()));
			break;
		
		case qeString:
			result.reset(IteratorForString("*", fToken));
			Match(qeString);
			break;
			
		case qeIdent:
		{
			string s = fToken;
			Match(qeIdent);

			if (fLookahead == ':' or (fLookahead >= qeLT and fLookahead <= qeGT))
				result.reset(Parse_QualifiedTest(s));
			else
			{
				if (fAutoWildcard)
				{
					if (s[s.length() - 1] != '*')
						s += '*';
					result.reset(IteratorForTerm("*", s, true));
				}
				else
					result.reset(IteratorForTerm("*", s, false));
			}
			break;
		}
		
		default:
			result.reset(Parse_Term("*"));
			break;
		
	}
	
	return result.release();
}

CDocIterator* CQueryImp::Parse_QualifiedTest(const string& inIndex)
{
	auto_ptr<CDocIterator> result(nil);
	
	switch (fLookahead)
	{
		case ':':
			Match(CQueryToken(':'));
			result.reset(Parse_Term(inIndex));
			break;
		
		case qeLT:
			Match(qeLT);
			result.reset(IteratorForOperator(inIndex, fToken, kOpLessThan));
			Match(qeIdent);
			break;
			
		case qeLE:
			Match(qeLE);
			result.reset(IteratorForOperator(inIndex, fToken, kOpLessEqual));
			Match(qeIdent);
			break;
			
		case qeEQ:
			Match(qeEQ);
			result.reset(IteratorForOperator(inIndex, fToken, kOpEquals));
			Match(qeIdent);
			break;
			
		case qeGT:
			Match(qeGT);
			result.reset(IteratorForOperator(inIndex, fToken, kOpGreaterThan));
			Match(qeIdent);
			break;
			
		case qeGE:
			Match(qeGE);
			result.reset(IteratorForOperator(inIndex, fToken, kOpGreaterEqual));
			Match(qeIdent);
			break;
		
		default:
			if (fAutoWildcard and fToken[fToken.length() - 1] != '*')
				fToken += '*';
			result.reset(IteratorForTerm("*", fToken, fAutoWildcard));
			Match(qeIdent);
			break;
	}
	
	return result.release();
}

CDocIterator* CQueryImp::Parse_Term(const string& inIndex)
{
	auto_ptr<CDocIterator> result(nil);
	
	switch (fLookahead)
	{
		case qeString:
			Match(qeString);
			result.reset(IteratorForTerm(inIndex, fToken, false));
			break;

		case qeIdent:
			if (fAutoWildcard)
			{
				if (fToken[fToken.length() - 1] != '*')
					fToken += '*';
				result.reset(IteratorForTerm(inIndex, fToken, true));
			}
			else
				result.reset(IteratorForTerm(inIndex, fToken, false));
			Match(qeIdent);
			break;
		
		case qePattern:
			result.reset(IteratorForTerm(inIndex, fToken, true));
			Match(qePattern);
			break;
		
		default:
			Match(qeIdent);	// force a parse error
			break;
	}
	
	return result.release();
}

CDocIterator*
CQueryImp::IteratorForTerm(const string& inIndex, const string& inTerm, bool inTermIsPattern)
{
	vector<CDocIterator*> ixs;
	
	for (uint32 ix = 0; ix < fIndexNames.size(); ++ix)
	{
		if (inIndex == "*" or inIndex == fIndexNames[ix])
		{
			ixs.push_back(
				fDatabank.CreateDocIterator(fIndexNames[ix], inTerm, inTermIsPattern, kOpContains));
		}
	}
	
	if (ixs.size() == 0)
		THROW(("Index %s does not exists in this databank", inIndex.c_str()));
	
	return CDocUnionIterator::Create(ixs);
}

CDocIterator*
CQueryImp::IteratorForOperator(const string& inIndex, const string& inKey,
	CQueryOperator inOperator)
{
	if (inIndex == "*")
		THROW(("Index cannot be a wildcard for relational operators"));
	
	return fDatabank.CreateDocIterator(inIndex, inKey, false, inOperator);
}

CDocIterator*
CQueryImp::IteratorForString(const string& inIndex, const string& inString)
{
	if (inIndex != "*")
		THROW(("string searching in a specific index is not supported yet, sorry"));

	CDocIterator* result = nil;
	
	CTokenizer<255> tok(inString.c_str(), inString.length());
	bool isWord, isNumber;
	CTokenizer<255>::EntryText s;
	vector<string> stringWords;
	
	while (tok.GetToken(s, isWord, isNumber))
	{
		if (not (isWord or isNumber) or s[0] == 0)
			continue;

		char* c;
		for (c = s; *c; ++c)
			*c = static_cast<char>(tolower(*c));

		if (c - s < kMaxKeySize)
		{
			stringWords.push_back(s);
			
			vector<CDocIterator*> ixs;
	
			for (uint32 ix = 0; ix < fIndexNames.size(); ++ix)
			{
				if (inIndex == "*" or inIndex == fIndexNames[ix])
				{
					ixs.push_back(
						fDatabank.CreateDocIterator(fIndexNames[ix], s, false, kOpContains));
				}
			}
			
			if (result == nil)
				result = CDocUnionIterator::Create(ixs);
			else
				result = CDocIntersectionIterator::Create(result, CDocUnionIterator::Create(ixs));
		}
	}
	
	return new CDbStringMatchIterator(fDatabank, stringWords, result);
}

// ----------------------------------------------------------------------------
//
//	QueryObject support
//

CMatchQueryObject::CMatchQueryObject(CDatabankBase& inDb,
		const string& inValue, const string& inIndex)
	: CQueryObject(inDb)
	, fValue(inValue)
	, fIndex(inIndex)
	, fRelOp(kOpContains)
	, fIsPattern(inValue.find('*') != string::npos or inValue.find('?') != string::npos)
{
}

CMatchQueryObject::CMatchQueryObject(CDatabankBase& inDb,
		const string& inValue, const string& inRelOp, const string& inIndex)
	: CQueryObject(inDb)
	, fValue(inValue)
	, fIndex(inIndex)
	, fIsPattern(inValue.find('*') != string::npos or inValue.find('?') != string::npos)
{
	if (inRelOp == ":")
		fRelOp = kOpContains;
	else if (inRelOp == "=" or inRelOp == "==")
		fRelOp = kOpEquals;
	else if (inRelOp == "<")
		fRelOp = kOpLessThan;
	else if (inRelOp == "<=")
		fRelOp = kOpLessEqual;
	else if (inRelOp == ">")
		fRelOp = kOpGreaterThan;
	else if (inRelOp == ">=")
		fRelOp = kOpGreaterEqual;
	else
		THROW(("Unknown relational operator '%s'", inRelOp.c_str()));
}

CDocIterator* CMatchQueryObject::Perform()
{
	CQueryImp parser(fDb);
	return parser.Build(fIndex, fValue, fRelOp);
}

// CNotQueryObject

CNotQueryObject::CNotQueryObject(CDatabankBase& inDb, boost::shared_ptr<CQueryObject> inChild)
	: CQueryObject(inDb)
	, fChild(inChild)
{
}

CDocIterator* CNotQueryObject::Perform()
{
	return new CDocNotIterator(fChild->Perform(), fDb.Count()); 
}

// CUnionQueryObject

CUnionQueryObject::CUnionQueryObject(CDatabankBase& inDb,
		boost::shared_ptr<CQueryObject> inObjectA, boost::shared_ptr<CQueryObject> inObjectB)
	: CQueryObject(inDb)
	, fChildA(inObjectA)
	, fChildB(inObjectB)
{
}

CDocIterator* CUnionQueryObject::Perform()
{
	return CDocUnionIterator::Create(fChildA->Perform(), fChildB->Perform());
}

// CIntersectionQueryObject

CIntersectionQueryObject::CIntersectionQueryObject(CDatabankBase& inDb,
		boost::shared_ptr<CQueryObject> inObjectA, boost::shared_ptr<CQueryObject> inObjectB)
	: CQueryObject(inDb)
	, fChildA(inObjectA)
	, fChildB(inObjectB)
{
}

CDocIterator* CIntersectionQueryObject::Perform()
{
	return CDocIntersectionIterator::Create(fChildA->Perform(), fChildB->Perform());
}

// CParsedQueryObject

CParsedQueryObject::CParsedQueryObject(CDatabankBase& inDb,
		const string& inQuery, bool inAutoWildcard)
	: CQueryObject(inDb)
	, fQuery(inQuery)
	, fAutoWildcard(inAutoWildcard)
{
}

CDocIterator* CParsedQueryObject::Perform()
{
	CQueryImp parser(fDb, fQuery, fAutoWildcard);
	return parser.Parse();
}
