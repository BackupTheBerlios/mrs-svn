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
 
#include <iostream>
#include <fstream>
#include <cerrno>
#include <cstdarg>

//#include "MRS.h"

#include "MRS_swig.h"
#include "getopt.h"

using namespace std;

int							gNrOfHits = 250;
string						gProgram;
bool						gProteinDB = true;
bool						gGapped = true;
vector<string>				gDbPaths;
string						gDbNameExtension;
string						gInput;

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

int main(int argc, const char* argv[])
{
	// scan the options

	string db, mrs_query, matrix;
	ofstream of;
	streambuf* outBuf = NULL;
	float expect = 10.0;
	bool filter = true, gapped = true;
	int wordsize = 3, gapOpen = 11, gapExtend = 1;
	
	matrix = "BLOSUM62";

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "a:p:d:i:o:e:F:vb:M:W:g:l:G:E:")) != -1)
	{
		switch (c)
		{
			case 'a':
				THREADS = atoi(optarg);
				break;
			
			case 'p':
				gProgram = optarg;
				break;
			
			case 'd':
				db = optarg;
				break;
			
			case 'i':
				gInput = optarg;
				break;

			case 'l':
				mrs_query = optarg;
				break;

			case 'M':
				matrix = optarg;
				break;

			case 'e':
				expect = atof(optarg);
				break;

			case 'F':
				filter = OptArgIsTrue(c, optarg);
				break;
				
			case 'W':
				wordsize = atoi(optarg);
				break;
			
			case 'g':
				gapped = OptArgIsTrue(c, optarg);
				break;
			
			case 'G':
				gapOpen = atoi(optarg);
				break;
			
			case 'E':
				gapExtend = atoi(optarg);
				break;
			
			case 'b':
				gNrOfHits = atoi(optarg);
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

	if (gProgram == "blastp" or gProgram == "blastx")
		gProteinDB = true;
	else if (gProgram == "blastn" or gProgram == "tblastx" or gProgram == "tblastn")
		gProteinDB = false;
	else
		error("unknown program: %s", gProgram.c_str());
	
	if (gProgram != "blastp")
		error("Unsupported program: %s", gProgram.c_str());
	
	MDatabank mrsDb(db);
	
	ifstream queryFile;
	queryFile.open(gInput.c_str());
	if (not queryFile.is_open())
		error("Could not open query file %s", gInput.c_str());

	string line;
	getline(queryFile, line);
	
	if (line.length() == 0 or line[0] != '>')
		error("Input file is not a FastA file");
	
	string sequence;
	
	while (not queryFile.eof())
	{
		getline(queryFile, line);
		for (string::iterator i = line.begin(); i != line.end(); ++i)
		{
			if (isspace(*i))
				line.erase(i);
		}
		sequence += line;
	}
	
	queryFile.close();
	
	auto_ptr<MQueryResults> query;
	auto_ptr<MBlastHits> hits;
	
	if (mrs_query.length())
	{
		query.reset(mrsDb.Find(mrs_query));
		hits.reset(query->Blast(sequence, matrix,
			wordsize, expect, filter, gapped, gapOpen, gapExtend));
	}
	else
		hits.reset(mrsDb.Blast(sequence, matrix,
			wordsize, expect, filter, gapped, gapOpen, gapExtend));

	if (hits.get())
	{
		cout << hits->ReportInXML();
	}

	if (outBuf)
		cout.rdbuf(outBuf);

	return 0;
}
