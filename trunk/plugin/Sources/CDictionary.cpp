/*	$Id: CDictionary.cpp 168 2006-11-03 11:14:54Z hekkel $
	Copyright Maarten L. Hekkelman
	Created Thursday August 25 2005 12:47:02
*/

#include "MRS.h"

#include <iostream>
#include <set>

#include "HGlobals.h"
#include "HFile.h"
#include "HError.h"

#include "CDictionary.h"
#include "CDatabank.h"
#include "CIterator.h"

using namespace std;

union CTransition
{
	struct
	{
		bool		last	: 1;
		uint32		dest	: 22;
		bool		term	: 1;
		uint8		attr	: 8;
	}				b;
	uint32			d;
};


namespace
{

const int32
	kMatchReward		= 1,
	kDeletePenalty		= -1,
	kInsertPenalty		= -4,
	kSubstitutePenalty	= -2,
	kTransposePenalty	= -2;

const uint32
	kMaxStringLength		= 256,
	kMaxChars				= 256,
	kHashTableSize			= (1 << 20),
	kHashTableElementSize	= (1 << 10),
	kMaxAutomatonSize		= (1 << 22);

typedef vector<CTransition>	CAutomaton;

const CTransition kNullTransition = {};

class CHashTable
{
  public:
					CHashTable();
					~CHashTable();
	
	uint32			Hash(CTransition* state, uint32 state_len);
	uint32			Lookup(CTransition* state, uint32 state_len, CAutomaton& automaton);
					
  private:

	struct bucket
	{
		uint32			addr;
		uint32			size;
		uint32			next;
	};

	uint32*			table;
	vector<bucket>	buckets;
	int32			last_pos;
};

CHashTable::CHashTable()
	: table(new uint32[kHashTableSize])
	, last_pos(-1)
{
	memset(table, 0, kHashTableSize * sizeof(uint32));
	buckets.push_back(bucket());	// dummy
}

CHashTable::~CHashTable()
{
	delete[] table;
}

uint32 CHashTable::Hash(CTransition* state, uint32 state_len)
{
	uint32 r = 0;

	for (uint32 i = 0; i < state_len; ++i)
		r += state[i].d;
	
	return ((r * 324027) >> 13) % kHashTableSize;
}

uint32 CHashTable::Lookup(CTransition* state, uint32 state_len, CAutomaton& automaton)
{
	if (state_len == 0)
		state[state_len++] = kNullTransition;
	
	state[state_len - 1].b.last = true;

	uint32 addr = Hash(state, state_len);
	bool found = false;
	
	for (uint32 ix = table[addr]; ix != 0 and not found; ix = buckets[ix].next)
	{
		if (buckets[ix].size == state_len)
		{
			found = true;
			
			for (uint32 i = 0; i < state_len; ++i)
			{
				if (automaton[buckets[ix].addr + i].d != state[i].d)
				{
					found = false;
					break;
				}
			}
			
			if (found)
				addr = buckets[ix].addr;
		}
	}
	
	if (not found)
	{
		uint32 size = automaton.size();
		
		for (uint32 i = 0; i < state_len; ++i)
			automaton.push_back(state[i]);
		
		bucket b;
		b.addr = size;
		b.size = state_len;
		b.next = table[addr];
		table[addr] = buckets.size();
		
		addr = size;
		
		buckets.push_back(b);
	}
	
	return addr;
}

const uint32
	kMaxScoreTableSize = 20,
	kMaxEdits = 2;

struct CScoreTableImp
{
	struct CScore
	{
		string		term;
		int32		score;
		
					CScore()
						: score(0) {}
					CScore(const CScore& inOther)
						: term(inOther.term)
						, score(inOther.score) {}
					CScore(string inTerm, int32 inScore)
						: term(inTerm)
						, score(inScore) {}
		
		bool		operator<(const CScore& inOther) const
						{ return score < inOther.score; }
		bool		operator>(const CScore& inOther) const
						{ return score > inOther.score; }
	};

					CScoreTableImp(
						const union CTransition*	inAutomaton,
						uint32						inAutomatonLength,
						const string&				inWord);

	void			Test(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);
	bool			Match(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);
	void			Delete(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);
	void			Insert(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);
	void			Transpose(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);
	void			Substitute(uint32 inState, int32 inScore, uint32 inEdits,
						string inMatch, const char* inWoord);

	void			Add(string inTerm, int32 inScore);
	void			Finish();
	int32			MinScore();

	const union CTransition*
					fAutomaton;
	uint32			fAutomatonLength;
	CScore			scores[kMaxScoreTableSize];
	uint32			n;
};

CScoreTableImp::CScoreTableImp(
	const union CTransition*	inAutomaton,
	uint32						inAutomatonLength,
	const string&				inWord)
	: fAutomaton(inAutomaton)
	, fAutomatonLength(inAutomatonLength)
	, n(0)
{
	string match;
	Test(fAutomaton[fAutomatonLength - 1].b.dest, 0, 0, match, inWord.c_str());

	Finish();
}

void CScoreTableImp::Test(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	Match(inState, inScore, inEdits, inMatch, inWord);

	if (inScore >= max(MinScore() - 3, 0) and inEdits < 3)
	{
		Delete(inState, inScore, inEdits, inMatch, inWord);
		Insert(inState, inScore, inEdits, inMatch, inWord);
		Transpose(inState, inScore, inEdits, inMatch, inWord);
		Substitute(inState, inScore, inEdits, inMatch, inWord);
	}
}

bool CScoreTableImp::Match(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	bool match = false;
	
	if (*inWord != 0)
	{
		match = true;
		
		while (fAutomaton[inState].b.attr != inWord[0])
		{
			if (fAutomaton[inState].b.last)
			{
				match = false;
				break;
			}
			
			++inState;
		}
		
		if (match)
		{
			inScore += kMatchReward;
			inMatch += inWord[0];
			
			if (fAutomaton[inState].b.term and inEdits < kMaxEdits)
				Add(inMatch, inScore);
			
			Test(fAutomaton[inState].b.dest, inScore, inEdits, inMatch, inWord + 1);
		}
	}
	
	return match;
}

void CScoreTableImp::Delete(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	uint32 state = inState;

	for (;;)
	{
		int8 ch = fAutomaton[state].b.attr;

		if (fAutomaton[state].b.term and inEdits < kMaxEdits)
			Add(inMatch + ch, inScore + kDeletePenalty);

		Test(fAutomaton[state].b.dest, inScore + kDeletePenalty, inEdits + 1, inMatch + ch, inWord);

		if (fAutomaton[state].b.last)
			break;

		++state;
	}
}

void CScoreTableImp::Insert(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	if (*inWord != 0)
		Test(inState, inScore + kInsertPenalty, inEdits + 1, inMatch, inWord + 1);
}

void CScoreTableImp::Transpose(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	if (inWord[0] != 0 and inWord[1] != 0)
	{
		string woord;
		woord = inWord[1];
		woord += inWord[0];
		woord += inWord + 2;
		Test(inState, inScore + kTransposePenalty, inEdits + 1, inMatch, woord.c_str());
	}
}

void CScoreTableImp::Substitute(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	if (inWord[0] != 0)
	{
		uint32 state = inState;
	
		for (;;)
		{
			int8 ch = fAutomaton[state].b.attr;

			if (fAutomaton[state].b.term and inEdits < kMaxEdits)
				Add(inMatch + ch, inScore + kSubstitutePenalty);

			Test(fAutomaton[state].b.dest, inScore + kSubstitutePenalty, inEdits + 1, inMatch + ch, inWord + 1);
			
			if (fAutomaton[state].b.last)
				break;

			++state;
		}
	}
}

void CScoreTableImp::Add(string inTerm, int32 inScore)
{
	if (n >= kMaxScoreTableSize)
	{
		if (inScore > scores[0].score)
		{
			pop_heap(scores, scores + n, greater<CScore>());
			scores[kMaxScoreTableSize - 1] = CScore(inTerm, inScore);
			push_heap(scores, scores + n, greater<CScore>());
		}
	}
	else
	{
		scores[n] = CScore(inTerm, inScore);
		++n;
		push_heap(scores, scores + n, greater<CScore>());
	}
}

void CScoreTableImp::Finish()
{
	sort_heap(scores, scores + n, greater<CScore>());
}

int32 CScoreTableImp::MinScore()
{
	int32 result = 0;
	if (n > 0)
		result = scores[0].score;
	return result;
}

}

struct CScoreTable : public CScoreTableImp {};

CDictionary::CDictionary(CDatabankBase& inDatabank)
	: fDatabank(inDatabank)
	, fDictionaryFile(nil)
	, fAutomaton(nil)
{
	HUrl url = inDatabank.GetDictionaryFileURL();

	if (not HFile::Exists(url))
		THROW(("Dictionary %s does not exist", url.GetURL().c_str()));

	if (VERBOSE > 0)
		cout << "Opening dictionary file " << url.GetURL() << endl;
	
	fDictionaryFile.reset(new HFileStream(url, O_RDONLY));

	fMemMapper.reset(new HMMappedFileStream(*fDictionaryFile.get(), 0,
		fDictionaryFile->Size()));
	
	fAutomaton = static_cast<const CTransition*>(fMemMapper->Buffer());
	fAutomatonLength = fDictionaryFile->Size() / sizeof(CTransition);
}

CDictionary::~CDictionary()
{
	fMemMapper.reset(nil);
	fDictionaryFile.reset(nil);
}

void CDictionary::Create(CDatabankBase& inDatabank,
	const vector<string>& inIndexNames,
	uint32 inMinOccurrence, uint32 inMinWordLength)
{
	CAutomaton automaton;
	
	unsigned char s0[kMaxStringLength] = "";
//	unsigned char s1[kMaxStringLength];
	
	CTransition larval_state[kMaxStringLength + 1][kMaxChars];
	uint32 l_state_len[kMaxStringLength + 1];
	bool is_terminal[kMaxStringLength];
	
	memset(l_state_len, 0, sizeof(uint32) * (kMaxStringLength + 1));
	
	CHashTable ht;
	
	uint32 i = 0, p, q;

	// now create the iterator for the indexes

	vector<CIteratorBase*> iters;
	for (vector<string>::const_iterator ix = inIndexNames.begin(); ix != inIndexNames.end(); ++ix)
	{
		if (VERBOSE >= 1)
			cout << "Adding index " << *ix << " to dictionary" << endl;
		iters.push_back(inDatabank.GetIteratorForIndex(*ix));
	}
	
	auto_ptr<CIteratorBase> iter(new CStrUnionIterator(iters));

	if (VERBOSE >= 1)
		cerr << "Creating dictionary" << endl;
	
	string s;
	int64 v;
	
	while (iter->Next(s, v))
	{
		q = s.length();
		
		if (q < inMinWordLength)
			continue;
		
		bool isNumber = true;
		for (string::iterator ch = s.begin(); isNumber and ch != s.end(); ++ch)
			isNumber = isdigit(*ch) or *ch == '.';

		if (isNumber)	// numbers should not be put into a dictionary, obviously
			continue;
		
		if (inMinOccurrence > 1)
		{
			bool accept = false;

			for (vector<string>::const_iterator i = inIndexNames.begin(); accept == false and i != inIndexNames.end(); ++i)
				accept = inDatabank.CountDocumentsContainingKey(*i, s) >= inMinOccurrence;

			if (not accept)
				continue;
		}

//		s.copy(s1, s.length() + 1);
		
		for (p = 0; s[p] == s0[p]; ++p)
			;
		
		if (s[p] < s0[p])
			THROW(("error, strings are unsorted"));
		
		while (i > p)
		{
			CTransition new_trans;

			new_trans.b.dest = ht.Lookup(larval_state[i], l_state_len[i], automaton);
			new_trans.b.term = is_terminal[i];
			new_trans.b.attr = s0[--i];
			
			larval_state[i][l_state_len[i]++] = new_trans;
		}
		
		while (i < q)
		{
			s0[i] = s[i];
			is_terminal[++i] = 0;
			l_state_len[i] = 0;
		}
		
		s0[q] = 0;
		is_terminal[q] = 1;
	}
	
	while (i > 0)
	{
		CTransition new_trans;

		new_trans.b.dest = ht.Lookup(larval_state[i], l_state_len[i], automaton);
		new_trans.b.term = is_terminal[i];
		new_trans.b.attr = s0[--i];
		
		larval_state[i][l_state_len[i]++] = new_trans;
	}
	
	uint32 start_state = ht.Lookup(larval_state[0], l_state_len[0], automaton);
	
	CTransition t = { };
	t.b.dest = start_state;
	automaton.push_back(t);
	
	HUrl url = inDatabank.GetDictionaryFileURL();
	HFileStream file(url, O_RDWR|O_CREAT|O_TRUNC);
	file.Write(&automaton[0], automaton.size() * sizeof(CTransition));
}

vector<string> CDictionary::SuggestCorrection(const string& inWord)
{
	CScoreTableImp scores(fAutomaton, fAutomatonLength, inWord);

	set<string> unique;
	vector<string> words;
	
	for (uint32 i = 0; i < scores.n; ++i)
	{
		string term = scores.scores[i].term;

		if (term != inWord and unique.find(term) == unique.end())
		{
			words.push_back(term);
			unique.insert(term);
		}
	}
	
	return words;
}

