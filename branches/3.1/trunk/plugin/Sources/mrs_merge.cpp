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
	cout << "Usage: mrs_merge -d databank -P part_count [-l] [-v]" << endl
	     << "       mrs_merge -d databank -p part1 [-p part2 ...] [-l] [-v]" << endl
	     << endl
	     << "    -l   link data instead of copying it" << endl;
	exit(1);
}

int main(int argc, const char* argv[])
{
	// scan the options

	string db;
	vector<string> partNames;
	int parts = 0;
	bool copy = true;

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "d:P:p:vl")) != -1)
	{
		switch (c)
		{
			case 'd':
				db = optarg;
				break;
			
			case 'P':
				parts = atoi(optarg);
				break;

			case 'p':
				partNames.push_back(optarg);
				break;

			case 'l':
				copy = false;
				break;

			case 'v':
				++VERBOSE;
				break;
			
			default:
				usage();
				break;
		}
	}

	if (db.length() == 0 or ((parts == 0) == (partNames.size() == 0)))
		usage();
	
	string merged = db;
	
	if (merged.find('/') == string::npos)
	{
		const char* data_dir = getenv("MRS_DATA_DIR");
		if (data_dir != NULL)
			merged = string(data_dir) + '/' + merged;
	}
		
	if (merged.substr(merged.length() - 4, 4) != ".cmp")
		merged += ".cmp";
	
	if (VERBOSE)
		cout << "Merging databank " << db << " creating " << merged << endl;
	
	MDatabankArray dbs;
	
	for (int i = 1; i <= parts; ++i)
	{
		stringstream s;
		s << db << '-' << i;
		partNames.push_back(s.str());
	}
	
	for (vector<string>::iterator n = partNames.begin(); n != partNames.end(); ++n)
		dbs.push_back(new MDatabank(*n));
	
	MDatabank::Merge(merged, dbs, copy, dbs.front()->GetName(),
		dbs.front()->GetInfoURL(), dbs.front()->GetScriptName(), 
		dbs.front()->GetSection());

	for (int i = 0; i < parts; ++i)
		delete dbs[i];
	
	return 0;
}
