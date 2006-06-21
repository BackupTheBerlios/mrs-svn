/*	$Id$
	Copyright Maarten L. Hekkelman
	Created Thursday August 25 2005 12:47:02
*/

#include "MRS.h"

#include <iostream>

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

}

CDictionary::CDictionary(CDatabankBase& inDatabank)
	: fDatabank(inDatabank)
	, fDictionaryFile(nil)
	, fAutomaton(nil)
{
	HUrl url = inDatabank.GetDictionaryFileURL();

	if (not HFile::Exists(url))
		THROW(("Dictionary %s does not exist", url.GetURL().c_str()));
	
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
			throw "error, strings are unsorted";
		
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
	fHits.clear();
	fCutOff = -8;
	
	vector<string> words;
	
	string match;
	Test(fAutomaton[fAutomatonLength - 1].b.dest, 0, 0, match, inWord.c_str());

	sort(fHits.begin(), fHits.end(), CSortOnScore());
	
	string last;
	int32 high = 0;
	
	for (CScores::iterator s = fHits.begin();
		s != fHits.end() and words.size() < 10; ++s)
	{
		if ((*s).second > high)
			high = (*s).second;
		else if ((*s).second < high + fCutOff)
			break;
		
		if ((*s).first != last and (*s).first != inWord)
			words.push_back((*s).first);

		if (VERBOSE >= 1)
			cout << "suggestion " << words.size() << ": " << (*s).first << "; score: " << (*s).second << endl;
		
		last = (*s).first;
	}
	
	return words;
}

void CDictionary::Test(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	Match(inState, inScore, inEdits, inMatch, inWord);

	if (inScore >= static_cast<int32>(inMatch.length()) + fCutOff and inEdits < 3)
	{
		Delete(inState, inScore, inEdits, inMatch, inWord);
		Insert(inState, inScore, inEdits, inMatch, inWord);
		Transpose(inState, inScore, inEdits, inMatch, inWord);
		Substitute(inState, inScore, inEdits, inMatch, inWord);
	}
}

bool CDictionary::Match(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
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
			
			if (fAutomaton[inState].b.term)
				AddSuggestion(inScore, inEdits, inMatch);
			
			Test(fAutomaton[inState].b.dest, inScore, inEdits, inMatch, inWord + 1);
		}
	}
	
	return match;
}

void CDictionary::Delete(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	uint32 state = inState;

	for (;;)
	{
		int8 ch = fAutomaton[state].b.attr;

		if (fAutomaton[state].b.term)
			AddSuggestion(inScore + kDeletePenalty, inEdits + 1, inMatch + ch);

		Test(fAutomaton[state].b.dest, inScore + kDeletePenalty, inEdits + 1, inMatch + ch, inWord);

		if (fAutomaton[state].b.last)
			break;

		++state;
	}
}

void CDictionary::Insert(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	if (*inWord != 0)
		Test(inState, inScore + kInsertPenalty, inEdits + 1, inMatch, inWord + 1);
}

void CDictionary::Transpose(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
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

void CDictionary::Substitute(uint32 inState, int32 inScore, uint32 inEdits, string inMatch, const char* inWord)
{
	if (inWord[0] != 0)
	{
		uint32 state = inState;
	
		for (;;)
		{
			int8 ch = fAutomaton[state].b.attr;

			if (fAutomaton[state].b.term)
				AddSuggestion(inScore + kSubstitutePenalty, inEdits + 1, inMatch + ch);

			Test(fAutomaton[state].b.dest, inScore + kSubstitutePenalty, inEdits + 1, inMatch + ch, inWord + 1);
			
			if (fAutomaton[state].b.last)
				break;
			++state;
		}
	}
}

void CDictionary::AddSuggestion(int32 inScore, uint32 inEdits, std::string inMatch)
{
	if (/*inScore >= static_cast<int32>(inMatch.length()) + fCutOff and */inEdits < 3)
	{
		pair<string, int32> v(inMatch, inScore);
		
		CScores::iterator i = lower_bound(fHits.begin(), fHits.end(), v, CSortOnWord());
		
		if (i != fHits.end() and (*i).first == inMatch)
			(*i).second = max((*i).second, inScore);
		else
			fHits.insert(i, v);
	}
}



