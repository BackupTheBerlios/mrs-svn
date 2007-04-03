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

#include "MRSInterface.h"
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

const char* kNest[] =
{
	"",
	"  ",
	"    ",
	"      ",
	"        ",
	"          ",
	"            ",
	"              ",
	"                ",
	"                  "
};

void Report(
	MDatabank&			inDb,
	const string&		inQueryID,
	const string&		inQueryDesc,
	unsigned long		inQueryLength,
	unsigned long		inReportLimit,
	const string&		inMatrix,
	double				inExpect,
	bool				inGapped,
	unsigned long		inGapOpen,
	unsigned long		inGapExtend,
	bool				inFilter,
	MBlastHits&			inBlastHits)
{
	cout << kNest[0] << "<?xml version=\"1.0\"?>" << endl;
	cout << kNest[0] << "<BlastOutput>" << endl;
	cout << kNest[1] << "<BlastOutput_program>blastp</BlastOutput_program>" << endl;
	cout << kNest[1] << "<BlastOutput_version>MRS blast</BlastOutput_version>" << endl;
	cout << kNest[1] << "<BlastOutput_reference>output generated by MRS blast, M.L. Hekkelman CMBI Radboud University Nijmegen, not submitted (yet).</BlastOutput_reference>" << endl;
	cout << kNest[1] << "<BlastOutput_db>" << inDb.GetName() << "</BlastOutput_db>" << endl;
	cout << kNest[1] << "<BlastOutput_query-ID>" << inQueryID << "</BlastOutput_query-ID>" << endl;
	cout << kNest[1] << "<BlastOutput_query-def>" << inQueryDesc << "</BlastOutput_query-def>" << endl;
	cout << kNest[1] << "<BlastOutput_query-len>" << inQueryLength << "</BlastOutput_query-len>" << endl;
	cout << kNest[1] << "<BlastOutput_param>" << endl;
	cout << kNest[2] << "<Parameters>" << endl;
	cout << kNest[3] << "<Parameters_matrix>" << inMatrix << "</Parameters_matrix>" << endl;
	cout << kNest[3] << "<Parameters_expect>" << inExpect << "</Parameters_expect>" << endl;
	if (inGapped)
	{
		cout << kNest[3] << "<Parameters_gap-open>" << inGapOpen << "</Parameters_gap-open>" << endl;
		cout << kNest[3] << "<Parameters_gap-extend>" << inGapExtend << "</Parameters_gap-extend>" << endl;
	}
	
	if (inFilter)
		cout << kNest[3] << "<Parameters_filter>S</Parameters_filter>" << endl;
	else
		cout << kNest[3] << "<Parameters_filter>F</Parameters_filter>" << endl;
	
	cout << kNest[2] << "</Parameters>" << endl;
	cout << kNest[1] << "</BlastOutput_param>" << endl;
	cout << kNest[1] << "<BlastOutput_iterations>" << endl;
	cout << kNest[2] << "<Iteration>" << endl;
	cout << kNest[3] << "<Iteration_iter-num>1</Iteration_iter-num>" << endl;
	
	// the hits
	
	cout << kNest[3] << "<Iteration_hits>" << endl;
	
	unsigned long n = 1;
	for (auto_ptr<MBlastHit> hit(inBlastHits.Next()); hit.get() != NULL and n <= inReportLimit; hit.reset(inBlastHits.Next()), ++n)
	{
		const char* id = hit->Id();
		if (id == NULL)
			error("Problem retrieving hit ID");
		
		const char* title = inDb.GetMetaData(id, "title");
		if (title == NULL)
			title = "";
		
		unsigned long hspNr = 1;
		auto_ptr<MBlastHsps> hsps(hit->Hsps());
		if (hsps.get() == NULL)
			error("No hsps in hit %d", n);
		
		auto_ptr<MBlastHsp> hsp(hsps->Next());
		if (hsp.get() == NULL)
			error("No hsp in hit %d", n);

		cout << kNest[4] << "<Hit>" << endl;
		cout << kNest[5] << "<Hit_num>" << n << "</Hit_num>" << endl;
		cout << kNest[5] << "<Hit_id>lcl|" << id << "</Hit_id>" << endl;
		cout << kNest[5] << "<Hit_def>" << title << "</Hit_def>" << endl;
//        <Hit_accession>sprot:CLPS_LEPIN</Hit_accession>
		cout << kNest[5] << "<Hit_accession>" << id << "</Hit_accession>" << endl;
		cout << kNest[5] << "<Hit_len>" << hsp->SubjectLength() << "</Hit_len>" << endl;
		cout << kNest[5] << "<Hit_hsps>" << endl;
		
		for (; hsp.get() != NULL; hsp.reset(hsps->Next()), ++hspNr)
		{
			string queryAlignment = hsp->QueryAlignment();
			string hitAlignment = hsp->SubjectAlignment();
			string midLine = hsp->Midline();
			
			unsigned long queryAlignmentLength = 0;
			for (string::iterator a = queryAlignment.begin(); a != queryAlignment.end(); ++a)
				if (*a != '-') queryAlignmentLength += 1;

			unsigned long hitAlignmentLength = 0;
			for (string::iterator a = hitAlignment.begin(); a != hitAlignment.end(); ++a)
				if (*a != '-') hitAlignmentLength += 1;
			
			cout << kNest[6] << "<Hsp>" << endl;
			cout << kNest[7] << "<Hsp_num>" << hspNr << "</Hsp_num>" << endl;
			cout << kNest[7] << "<Hsp_bit-score>" << hsp->BitScore() << "</Hsp_bit-score>" << endl;
			cout << kNest[7] << "<Hsp_score>" << hsp->Score() << "</Hsp_score>" << endl;
			cout << kNest[7] << "<Hsp_evalue>" << hsp->Expect() << "</Hsp_evalue>" << endl;
			cout << kNest[7] << "<Hsp_query-from>" << hsp->QueryStart() + 1 << "</Hsp_query-from>" << endl;
			cout << kNest[7] << "<Hsp_query-to>" << hsp->QueryStart() + queryAlignmentLength + 1 << "</Hsp_query-to>" << endl;
			cout << kNest[7] << "<Hsp_hit-from>" << hsp->SubjectStart() + 1 << "</Hsp_hit-from>" << endl;
			cout << kNest[7] << "<Hsp_hit-to>" << hsp->SubjectStart() + hitAlignmentLength + 1 << "</Hsp_hit-to>" << endl;
//			cout << kNest[7] << "<Hsp_query-frame>1</Hsp_query-frame>" << endl;
//			cout << kNest[7] << "<Hsp_hit-frame>1</Hsp_hit-frame>" << endl;
			cout << kNest[7] << "<Hsp_identity>" << hsp->Identity() << "</Hsp_identity>" << endl;
			cout << kNest[7] << "<Hsp_positive>" << hsp->Positive() << "</Hsp_positive>" << endl;
			if (hsp->Gaps() > 0)
				cout << kNest[7] << "<Hsp_gaps>" << hsp->Gaps() << "</Hsp_gaps>" << endl;
			cout << kNest[7] << "<Hsp_align-len>" << midLine.length() << "</Hsp_align-len>" << endl;
			cout << kNest[7] << "<Hsp_qseq>" << queryAlignment << "</Hsp_qseq>" << endl;
			cout << kNest[7] << "<Hsp_hseq>" << hitAlignment << "</Hsp_hseq>" << endl;
			cout << kNest[7] << "<Hsp_midline>" << midLine << "</Hsp_midline>" << endl;
			cout << kNest[6] << "</Hsp>" << endl;
		}
		
		cout << kNest[5] << "</Hit_hsps>" << endl;
		cout << kNest[4] << "</Hit>" << endl;
	}
	
	cout << kNest[3] << "</Iteration_hits>" << endl;
	cout << kNest[3] << "<Iteration_stat>" << endl;
	
	// statistics
	
	cout << kNest[4] << "<Statistics>" << endl;
	cout << kNest[5] << "<Statistics_db-num>" << inBlastHits.DbCount() << "</Statistics_db-num>" << endl;
	cout << kNest[5] << "<Statistics_db-len>" << inBlastHits.DbLength() << "</Statistics_db-len>" << endl;
//	cout << kNest[5] << "<Statistics_hsp-len>0</Statistics_hsp-len>" << endl;
	cout << kNest[5] << "<Statistics_eff-space>" << inBlastHits.EffectiveSpace() << "</Statistics_eff-space>" << endl;
	cout << kNest[5] << "<Statistics_kappa>" << inBlastHits.Kappa() << "</Statistics_kappa>" << endl;
	cout << kNest[5] << "<Statistics_lambda>" << inBlastHits.Lambda() << "</Statistics_lambda>" << endl;
	cout << kNest[5] << "<Statistics_entropy>" << inBlastHits.Entropy() << "</Statistics_entropy>" << endl;
	cout << kNest[4] << "</Statistics>" << endl;
	cout << kNest[3] << "</Iteration_stat>" << endl;
	cout << kNest[2] << "</Iteration>" << endl;
	cout << kNest[1] << "</BlastOutput_iterations>" << endl;
	cout << kNest[0] << "</BlastOutput>" << endl;
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

	string id, desc;
	string::size_type sp = line.find(' ', 1);
	if (sp != string::npos)
	{
		id = line.substr(1, sp - 1);
		desc = line.substr(sp + 1);
	}
	else
		id = line.substr(1);
	
	string sequence;
	
	while (not queryFile.eof())
	{
		getline(queryFile, line);
		
		if (line.length() > 0 and line[0] == '>')
			error("only one sequence per input file is supported");
		
		for (string::iterator i = line.begin(); i != line.end(); ++i)
		{
			if (isspace(*i))
				i = line.erase(i);
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
		Report(mrsDb, id, desc, sequence.length(), gNrOfHits,  matrix, expect, gapped, gapOpen, gapExtend, filter, *hits.get());

	if (outBuf)
		cout.rdbuf(outBuf);

	return 0;
}
