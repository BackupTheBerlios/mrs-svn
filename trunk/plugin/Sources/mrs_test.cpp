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

#include "MRS_swig.h"
#include "getopt.h"

#include <expat.h>

#define not !
#define and &&
#define or ||

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

static bool OptArgIsTrue(int inOption, const char* inOptArg)
{
	bool result = true;
	
	if (inOptArg != NULL)
	{
		if (strlen(inOptArg) != 1)
			error("argument to option %c should either be T or F", inOption);
		
		result = toupper(*inOptArg) == 'T';
	}
	
	return result;
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

	string db, mrs_query;
	ofstream of;
	streambuf* outBuf = NULL;

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "d:o:ve:f:s:")) != -1)
	{
		switch (c)
		{
			case 'd':
				db = optarg;
				break;
			
			case 'e':
				mrs_query = optarg;
				break;

			case 'v':
				++VERBOSE;
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
	
	stringstream path;
	path << "C:/data/fp/" << db << '/' << mrs_query << ".xml";

	ifstream fs(path.str().c_str());

	if (not fs.is_open())
		exit(1);

	vector<pair<string,float> > fp;

	XML_Parser p = XML_ParserCreate(NULL);
	if (p == NULL)
		exit(1);

	XML_SetUserData(p, &fp);

	XML_SetElementHandler(p, StartElement, EndElement);
//	XML_SetCharacterDataHandler(p, charData);
	
	while (not fs.eof())
	{
		string line;
		getline(fs, line);

		(void)XML_Parse(p, line.c_str(), line.length(), fs.eof());
	}
	
	XML_ParserFree(p);
	
	fs.close();
	
	auto_ptr<MRankedQuery> q(mrsDb.RankedQuery("mesh2002"));
	float minF = fp.back().second;
	for (vector<pair<string,float> >::iterator t = fp.begin(); t != fp.end(); ++t)
		q->AddTerm(t->first, static_cast<unsigned long>(t->second / minF));
	
	auto_ptr<MQueryResults> r(q->Perform());

	while (const char* id = r->Next())
		cout << id << endl;

	if (outBuf)
		cout.rdbuf(outBuf);

	return 0;
}
