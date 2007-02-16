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
 
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdarg>

//#include "MRS.h"

#include "MRSInterface.h"
#include "getopt.h"

using namespace std;

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
	ofstream of;
	streambuf* outBuf = NULL;
	vector<string> queryWords;

	ix = "__ALL_TEXT__";

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "d:o:vq:f:s:i:a:")) != -1)
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

			case 'o':
				of.open(optarg, ios::out | ios::trunc);
				outBuf = cout.rdbuf(of.rdbuf());
				break;
			
			default:
				usage();
				break;
		}
	}
	
	MDatabank mrsDb(db);
	
	auto_ptr<MQueryResults> r;

	if (queryWords.size() > 0)
	{
		auto_ptr<MRankedQuery> q(mrsDb.RankedQuery(ix));
	
		q->SetAlgorithm(alg);
		q->SetAllTermsRequired(1);
		q->SetMaxReturn(10);
	
		for (vector<string>::iterator qw = queryWords.begin(); qw != queryWords.end(); ++qw)
			q->AddTerm(*qw, 1);
		
		auto_ptr<MBooleanQuery> m;
		if (filter.length())
				m.reset(mrsDb.BooleanQuery(filter));
	
		r.reset(q->Perform(m.get()));
	}
	else if (filter.length() > 0)
		r.reset(mrsDb.Find(filter, false));
	
	if (r.get() != NULL)
	{
		stringstream s;
		
		int n = 10;
		const char* id;
		while (n-- > 0 and (id = r->Next()) != NULL)
		{
			const char* title = mrsDb.GetMetaData(id, "title");
			string desc;
			if (title != NULL)
				desc = title;
			s << id << '\t' << r->Score() << '\t' << desc << endl;
		}

		unsigned long count = r->Count(true);
		cout << "Found " << count << " hits, displaying the first " << min(10UL, count) << endl;
		cout << s.str() << endl;
		
	}
	else
		cout << "No hits found" << endl;

	if (outBuf)
		cout.rdbuf(outBuf);

	return 0;
}
