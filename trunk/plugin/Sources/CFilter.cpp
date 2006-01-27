/*	$Id: CFilter.cpp,v 1.2 2005/08/22 12:38:07 maarten Exp $
	Copyright Maarten Hekkelman, CMBI
	Created Monday April 26 2004 12:00:11
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

#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include <cctype>
#include <iostream>
#include <fstream>
#include <map>

#include "CFilter.h"

using namespace std;

class CAlphabet
{
  public:
					CAlphabet(const char* inChars);
	
	bool			Contains(char inChar) const;
	long			GetIndex(char inChar) const;
	long			GetSize() const			{ return fAlphaSize; }
	double			GetLnSize() const		{ return fAlphaLnSize; }

  private:
	int				fAlphaSize;
	double			fAlphaLnSize;
	long			fAlphaIndex[128];
	const char*		fAlphaChars;
					
	
	struct Compare
	{
		Compare(char c) : c(c) {}
		bool operator()(char inA) const {
			return toupper(inA) == toupper(c);
		}
		char c;
	};
};

CAlphabet::CAlphabet(const char* inChars)
	: fAlphaChars(inChars)
{
	fAlphaSize = strlen(inChars);
	fAlphaLnSize = log(static_cast<double>(fAlphaSize));
	
	for (uint32 i = 0; i < 128; ++i)
		fAlphaIndex[i] =
			find_if(fAlphaChars, fAlphaChars + fAlphaSize, Compare(i)) - fAlphaChars;
}

bool CAlphabet::Contains(char inChar) const
{
	bool result = false;
	if (inChar >= 0)
		result = fAlphaIndex[toupper(inChar)] < fAlphaSize;
	return result;
}

long CAlphabet::GetIndex(char inChar) const
{
	return fAlphaIndex[toupper(inChar)];
}

const CAlphabet
	kProtAlphabet = CAlphabet("acdefghiklmnpqrstvwy"),
	kNuclAlphabet = CAlphabet("acgtu");

class CWindow
{
  public:
			CWindow(string& inSequence, long inStart, long inLength, const CAlphabet& inAlphabet);

	void	CalcEntropy();
	bool	ShiftWindow();
	
	double	GetEntropy() const			{ return fEntropy; }
	long	GetBogus() const			{ return fBogus; }
	
	void	DecState(long inCount);
	void	IncState(long inCount);
	
	void	Trim(long& ioEndL, long& ioEndR, long inMaxTrim);

  private:
	string&			fSequence;
	vector<long>	fComposition;
	vector<long>	fState;
	long			fStart;
	long			fLength;
	long			fBogus;
	double			fEntropy;
	const CAlphabet&fAlphabet;
};

CWindow::CWindow(string& inSequence, long inStart, long inLength, const CAlphabet& inAlphabet)
	: fSequence(inSequence)
	, fStart(inStart)
	, fLength(inLength)
	, fBogus(0)
	, fEntropy(-2.0)
	, fAlphabet(inAlphabet)
{
	long alphaSize = fAlphabet.GetSize();
	
	fComposition.insert(fComposition.begin(), alphaSize, 0);
	for (uint32 i = fStart; i < fStart + fLength; ++i)
	{
		if (fAlphabet.Contains(fSequence[i]))
			++fComposition[fAlphabet.GetIndex(fSequence[i])];
		else
			++fBogus;
	}
	
	fState.insert(fState.begin(), alphaSize + 1, 0);

	int n = 0;
	for (uint32 i = 0; i < alphaSize; ++i)
	{
		if (fComposition[i] > 0)
		{
			fState[n] = fComposition[i];
			++n;
		}
	}
	
	sort(fState.begin(), fState.begin() + n, greater<long>());
}

void CWindow::CalcEntropy()
{
	fEntropy = 0.0;

	double total = 0.0;
	for (uint32 i = 0; i < fState.size() and fState[i] != 0; ++i)
		total += fState[i];

	if (total != 0.0)
	{
		for (uint32 i = 0; i < fState.size() and fState[i]; ++i)
		{
			double t = fState[i] / total;
			fEntropy += t * log(t);
		}
		fEntropy = fabs(fEntropy / log(0.5));
	}
}

void CWindow::DecState(long inClass)
{
	for (uint32 ix = 0; ix < fState.size() and fState[ix] != 0; ++ix)
	{
		if (fState[ix] == inClass and fState[ix + 1] < inClass)
		{
			--fState[ix];
			break;
		}
	}
}

void CWindow::IncState(long inClass)
{
	for (uint32 ix = 0; ix < fState.size(); ++ix)
	{
		if (fState[ix] == inClass)
		{
			++fState[ix];
			break;
		}
	}
}

bool CWindow::ShiftWindow()
{
	if (fStart + fLength >= fSequence.length())
		return false;
	
	char ch = fSequence[fStart];
	if (fAlphabet.Contains(ch))
	{
		long ix = fAlphabet.GetIndex(ch);
		DecState(fComposition[ix]);
		--fComposition[ix];
	}
	else
		--fBogus;
	
	++fStart;
	
	ch = fSequence[fStart + fLength - 1];
	if (fAlphabet.Contains(ch))
	{
		long ix = fAlphabet.GetIndex(ch);
		IncState(fComposition[ix]);
		++fComposition[ix];
	}
	else
		++fBogus;

	if (fEntropy > -2.0)
		CalcEntropy();
	
	return true;
}

static double lnfac(long inN)
{
    const double c[] = {
         76.18009172947146,
        -86.50532032941677,
         24.01409824083091,
        -1.231739572450155,
         0.1208650973866179e-2,
        -0.5395239384953e-5
    };
	static map<long,double> sLnFacMap;
	
	if (sLnFacMap.find(inN) == sLnFacMap.end())
	{
		double x = inN + 1;
		double t = x + 5.5;
		t -= (x + 0.5) * log(t);
	    double ser = 1.000000000190015; 
	    for (int i = 0; i <= 5; i++)
	    {
	    	++x;
	        ser += c[i] / x;
	    }
	    sLnFacMap[inN] = -t + log(2.5066282746310005 * ser / (inN + 1));
	}
	
	return sLnFacMap[inN];
}

static double lnperm(vector<long>& inState, long inTotal)
{
	double ans = lnfac(inTotal);
	for (uint32 i = 0; i < inState.size() and inState[i] != 0; ++i)
		ans -= lnfac(inState[i]);
	return ans;
}

static double lnass(vector<long>& inState, CAlphabet inAlphabet)
{
    double result = lnfac(inAlphabet.GetSize());
    if (inState.size() == 0 or inState[0] == 0)
        return result;
    
    int total = inAlphabet.GetSize();
    int cl = 1;
    int i = 1;
    int sv_cl = inState[0];
    
    while (inState[i] != 0)
    {
        if (inState[i] == sv_cl) 
            cl++;
        else
        {
            total -= cl;
            result -= lnfac(cl);
            sv_cl = inState[i];
            cl = 1;
        }
        i++;
    }

    result -= lnfac(cl);
    total -= cl;
    if (total > 0)
    	result -= lnfac(total);
    
    return result;
}

static double lnprob(vector<long>& inState, long inTotal, const CAlphabet& inAlphabet)
{
	double ans1, ans2 = 0, totseq;

	totseq = inTotal * inAlphabet.GetLnSize();
	ans1 = lnass(inState, inAlphabet);
	if (ans1 > -100000.0 and inState[0] != numeric_limits<long>::min())
		ans2 = lnperm(inState, inTotal);
	else
		cerr << "Error in calculating lnass" << endl;
	return ans1 + ans2 - totseq;
}

void CWindow::Trim(long& ioEndL, long& ioEndR, long inMaxTrim)
{
	double minprob = 1.0;
	long lEnd = 0;
	long rEnd = fLength - 1;
	int minLen = 1;
	int maxTrim = inMaxTrim;
	if (minLen < fLength - maxTrim)
		minLen = fLength - maxTrim;
	
	for (long len = fLength; len > minLen; --len)
	{
		CWindow w(fSequence, fStart, len, fAlphabet);
		
		int i = 0;
		bool shift = true;
		while (shift)
		{
			double prob = lnprob(w.fState, len, fAlphabet);
			if (prob < minprob)
			{
				minprob = prob;
				lEnd = i;
				rEnd = len + i - 1;
			}
			shift = w.ShiftWindow();
			++i;
		}
	}

	ioEndL += lEnd;
	ioEndR -= fLength - rEnd - 1;
}

static bool GetEntropy(string inSequence, const CAlphabet& inAlphabet,
	long inWindow, long inMaxBogus, vector<double>& outEntropy)
{
	bool result = false;

	long downset = (inWindow + 1) / 2 - 1;
	long upset = inWindow - downset;
	
	if (inWindow <= inSequence.length())
	{
		result = true;
		outEntropy.clear();
		outEntropy.insert(outEntropy.begin(), inSequence.length(), -1.0);
		
		CWindow win(inSequence, 0, inWindow, inAlphabet);
		win.CalcEntropy();
		
		long first = downset;
		long last = inSequence.length() - upset;
		for (long i = first; i <= last; ++i)
		{
//			if (GetPunctuation() and win.HasDash())
//			{
//				win.ShiftWindow();
//				continue;
//			}
			if (win.GetBogus() > inMaxBogus)
				continue;
			
			outEntropy[i] = win.GetEntropy();
			win.ShiftWindow();
		}
	}
	
	return result;
}

static void GetMaskSegments(bool inProtein, string inSequence, long inOffset,
	vector<pair<long,long> >& outSegments)
{
	double loCut, hiCut;
	long window, maxbogus, maxtrim;
	bool overlaps;
	const CAlphabet* alphabet;

	if (inProtein)
	{
		window = 12;
		loCut = 2.2;
		hiCut = 2.5;
		overlaps = false;
		maxtrim = 50;
		maxbogus = 2;
		alphabet = &kProtAlphabet;
	}
	else
	{
		window = 32;
		loCut = 1.4;
		hiCut = 1.6;
		overlaps = false;
		maxtrim = 100;
		maxbogus = 3;
		alphabet = &kNuclAlphabet;
	}

	long downset = (window + 1) / 2 - 1;
	long upset = window - downset;
	
	vector<double> e;
	GetEntropy(inSequence, *alphabet, window, maxbogus, e);

	long first = downset;
	long last = inSequence.length() - upset;
	long lowlim = first;

	for (long i = first; i <= last; ++i)
	{
		if (e[i] <= loCut and e[i] != -1.0)
		{
			long loi = i;
			while (loi >= lowlim and e[loi] != -1.0 and e[loi] <= hiCut)
				--loi;
			++loi;
			
			long hii = i;
			while (hii <= last and e[hii] != -1.0 and e[hii] <= hiCut)
				++hii;
			--hii;
			
			long leftend = loi - downset;
			long rightend = hii + upset - 1;
			
			string s(inSequence.substr(leftend, rightend - leftend + 1));
			CWindow w(s, 0, rightend - leftend + 1, *alphabet);
			w.Trim(leftend, rightend, maxtrim);

			if (i + upset - 1 < leftend)
			{
				long lend = loi - downset;
				long rend = leftend - 1;
				
				string left(inSequence.substr(lend, rend - lend + 1));
				GetMaskSegments(inProtein, left, inOffset + lend, outSegments);
			}
			
			outSegments.push_back(
				pair<long,long>(leftend + inOffset, rightend + inOffset + 1));
			i = rightend + downset;
			if (i > hii)
				i = hii;
			lowlim = i + 1;
		}
	}
}

string SEG(const string& inSequence)
{
	string result = inSequence;
	
	vector<pair<long,long> > segments;
	GetMaskSegments(true, result, 0, segments);
	
	for (uint32 i = 0; i < segments.size(); ++i)
	{
		for (uint32 j = segments[i].first; j < segments[i].second; ++j)
			result[j] = 'X';
	}
	
	return result;
}

string DUST(const string& inSequence)
{
	string result = inSequence;
	
	vector<pair<long,long> > segments;
	GetMaskSegments(false, inSequence, 0, segments);
	
	for (uint32 i = 0; i < segments.size(); ++i)
	{
		for (uint32 j = segments[i].first; j < segments[i].second; ++j)
			result[j] = 'X';
	}
	
	return result;
}

//int main()
//{
//	string seq;
//	
//	ifstream in("input.seq", ios::binary);
//	in >> seq;
//	cout << FilterProtSeq(seq);
//	return 0;
//}
