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

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdarg>

#include <sys/times.h>
#include <sys/resource.h>

#include "CDatabank.h"
#include "CQuery.h"
#include "CRankedQuery.h"

#include "getopt.h"

using namespace std;

namespace {
	
ostream& operator<<(ostream& inStream, const struct timeval& t)
{
	uint32 hours = t.tv_sec / 3600;
	uint32 minutes = ((t.tv_sec % 3600) / 60);
	uint32 seconds = t.tv_sec % 60;
	
	uint32 milliseconds = t.tv_usec / 1000;
	
	inStream << hours << ':'
			 << setw(2) << setfill('0') << minutes << ':'
			 << setw(2) << setfill('0') << seconds << '.'
			 << setw(3) << setfill('0') << milliseconds;
	
	return inStream;
}

class CStopwatch
{
  public:
			CStopwatch(double& ioAccumulator);
			~CStopwatch();

  private:
	double&			fAccumulator;
	struct rusage	fStartTime;
};

CStopwatch::CStopwatch(double& ioAccumulator)
	: fAccumulator(ioAccumulator)
{
	if (VERBOSE > 1)
		getrusage(RUSAGE_SELF, &fStartTime);
}

CStopwatch::~CStopwatch()
{
	if (VERBOSE > 1)
	{
		struct rusage stop;
		getrusage(RUSAGE_SELF, &stop);
		
		fAccumulator += (stop.ru_utime.tv_sec - fStartTime.ru_utime.tv_sec);
		fAccumulator += 0.000001 * (stop.ru_utime.tv_usec - fStartTime.ru_utime.tv_usec);
	}
}

static void PrintStatistics()
{
	struct rusage ru = {} ;
	
	if (getrusage(RUSAGE_SELF, &ru) < 0)
		cerr << "Error calling getrusage" << endl;
	
	cout << "Total time user:    " << ru.ru_utime << endl;
	cout << "Total time system:  " << ru.ru_stime << endl;
	cout << "I/O operations in:  " << ru.ru_inblock << endl;
	cout << "I/O operations out: " << ru.ru_oublock << endl;
}	
	
}

extern double system_time();

void error(const char* msg, ...)
{
	va_list vl;
	
	va_start(vl, msg);
	vfprintf(stderr, msg, vl);
	va_end(vl);

	fprintf(stderr, "\n");
	
	if (errno)
		fprintf(stderr, strerror(errno));
	
	fprintf(stderr, "\n");
	
	exit(1);
}

void usage()
{
	cout << "usage: mrs-query -d databank [-q term] [-f filter]" << endl
		 << "        -d databank     The databank to search" << endl
		 << "        -q term         A search term, can be repeated" << endl
		 << "        -f \"filter ...\" A boolean filter query" << endl
		 << endl
		 << "  additional parameters:" << endl
		 << "        -v              Verbose, repeat for increased output" << endl
		 << "        -n #maxreturn   Maximum number of lines of output" << endl
		 << "        -a algorithm    Ranking algorithm, default is 'vector'," << endl
		 << "                        alternatives are 'dice' and 'jaccard'." << endl
		 << "        -o output       Write output to file 'output'" << endl
		 << endl;
	
	exit(1);
}

string GetAttribute(string inName, const char** inAttr)
{
	string result;
	for (; *inAttr != NULL; inAttr += 2)
	{
		if (inName == *inAttr)
		{
			result = *++inAttr;
			break;
		}
	}
	return result;
}

void StartElement(void* inUserData, const char* inName, const char** inAtts)
{
	vector<pair<string,float> >& fp = *reinterpret_cast<vector<pair<string,float> >*>(inUserData);
	
	string id = GetAttribute("id", inAtts);
	if (id.length())
	{
		fp.push_back(make_pair<string,float>(id, atof(GetAttribute("rank", inAtts).c_str())));
	}
}

void EndElement(void *inUserData, const char *inName)
{
}

int main(int argc, const char* argv[])
{
	// scan the options

	string db, filter, ix, alg = "vector";
	uint32 max_output = 10;
	ofstream of;
	streambuf* outBuf = NULL;
	vector<string> queryWords;

	ix = "__ALL_TEXT__";

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "d:o:vq:f:s:i:a:n:")) != -1)
	{
		switch (c)
		{
			case 'd':
				db = optarg;
				break;
			
			case 'q':
				queryWords.push_back(optarg);
				break;

			case 'i':
				ix = optarg;
				break;

			case 'v':
				++VERBOSE;
				break;

			case 'a':
				alg = optarg;
				break;

			case 'f':
				filter = optarg;
				break;

			case 'n':
				max_output = atoi(optarg);
				break;
	
			case 'o':
				of.open(optarg, ios::out | ios::trunc);
				outBuf = cout.rdbuf(of.rdbuf());
				break;
			
			default:
				usage();
				break;
		}
	}
	
	if (db.length() == 0 or (queryWords.size() == 0 and filter.length() == 0))
		usage();
	
	double queryTime = 0, printTime = 0;
	double realQueryTime, realPrintTime;
	double now;

	HUrl url(db);
	CDatabank mrsDb(url);
	
	now = system_time();

	if (VERBOSE > 1)
		cout << "databank opened, building query" << endl;

	auto_ptr<CDocIterator> r;

	if (filter.length() > 0)
	{
		CStopwatch sw(queryTime);

		CParsedQueryObject p(mrsDb, filter, false);
		r.reset(p.Perform());
	}

	uint32 count = 0;

	if (queryWords.size() > 0)
	{
		CStopwatch sw(queryTime);

		CRankedQuery q;
	
		for (vector<string>::iterator qw = queryWords.begin(); qw != queryWords.end(); ++qw)
			q.AddTerm(*qw, 1);
		
		if (VERBOSE > 1)
			cout << "quiry built, performing" << endl;
		
		CDocIterator* qr;
		q.PerformSearch(mrsDb, ix, alg, r.get(), max_output, true, qr, count);
		r.reset(qr);
	}
	
	realQueryTime = system_time() - now;
	now = system_time();

	if (VERBOSE > 1)
		cout << "query done, printing results" << endl;

	if (r.get() != NULL)
	{
		CStopwatch sw(printTime);

		stringstream s;
		
		uint32 n = max_output, n2 = 0;
		if (n > count and count != 0)
			n = count;

		uint32 docNr;
		float score;

		while (n-- > 0 and r->Next(docNr, score, false))
		{
			s	<< mrsDb.GetDocumentID(docNr) << '\t'
				<< setprecision(3) << score << '\t'
				<< mrsDb.GetMetaData(docNr, "title")
				<< endl;
			++n2;
		}
		
		if (count == 0)
		{
			count = n2;
			while (r->Next(docNr, score, false))
				++count;
		}

		cout << "Found " << count << " hits, displaying the first " << min(max_output, count) << endl
			 << s.str()
			 << endl;
	}
	else
		cout << "No hits found" << endl;

	realPrintTime = system_time() - now;

	if (outBuf)
		cout.rdbuf(outBuf);

	if (VERBOSE > 1)
	{
		cout << "Time spent on" << endl;
	
		struct timeval t, tr;
		
		t.tv_sec = static_cast<uint32>(queryTime);
		t.tv_usec = static_cast<uint32>(queryTime * 1000000.0) % 1000000;
		
		tr.tv_sec = static_cast<uint32>(realQueryTime);
		tr.tv_usec = static_cast<uint32>(realQueryTime * 1000000.0) % 1000000;
		
		cout << "Query:      " << t << "     " << tr << endl;
	
		t.tv_sec = static_cast<uint32>(printTime);
		t.tv_usec = static_cast<uint32>(printTime * 1000000.0) % 1000000;

		tr.tv_sec = static_cast<uint32>(realPrintTime);
		tr.tv_usec = static_cast<uint32>(realPrintTime * 1000000.0) % 1000000;

		cout << "Printing:   " << t << "     " << tr << endl;
		
		cout << "total:" << endl;

		PrintStatistics();
	}

	return 0;
}
