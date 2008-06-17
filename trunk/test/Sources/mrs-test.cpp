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
#include <fstream>
#include <sstream>
#include <cerrno>
#include <cstdarg>

#include <boost/filesystem.hpp>


#include "HFile.h"

#include "CDatabank.h"
#include "CIndexer.h"
#include "CIndex.h"
#include "getopt.h"

using namespace std;
namespace fs = boost::filesystem;

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
	cout << "Usage: mrs-test -d databank [-v]" << endl
	     << "         -d databank   Must be the full path to the databank" << endl
		 << "                       runs a test on the indices in the databank" << endl
	     << endl
		 << "       mrs-test -d databank -k key -i index [-v]" << endl
	     << "         -k key        A key to search in the databank" << endl
	     << "         -i index      The index to search in the databank" << endl
	     << endl;
	exit(1);
}

int main(int argc, const char* argv[])
{
	// scan the options

	string db, index, key;

	int c;
	while ((c = getopt(argc, const_cast<char**>(argv), "d:vk:i:")) != -1)
	{
		switch (c)
		{
			case 'd':
				db = optarg;
				break;
			
			case 'k':
				key = optarg;
				break;

			case 'i':
				index = optarg;
				break;

			case 'v':
				++VERBOSE;
				break;
			
			default:
				usage();
				break;
		}
	}

	if (db.length() == 0 or ((index.length() == 0) != (key.length() == 0)))
		usage();
	
	if (not fs::exists(db))
		cerr << "please specify full path to existing db" << endl;
	else
	{
		try
		{
			HUrl url(db);
			CDatabank databank(url);
			const CIndexer* ixr = databank.GetIndexer();
	
			if (index.length() and key.length())
			{
#if 0
				auto_ptr<CIndex> ix(ixr->GetIndex(index));
	
				CIndex::iterator iter = ix->lower_bound(key);
				while (iter != ix->end())
				{
					cout << iter->first << ", " << iter->second << endl;
					++iter;
				}
#endif
				auto_ptr<CDocIterator> iter(ixr->CreateDocIterator(index, key, false, kOpContains));
				
				uint32 docNr = 0;
				while (iter->Next(docNr, false))
					;
			}
			else
				ixr->Test();
		}
		catch (exception& e)
		{
			cerr << "Exception: " << e.what() << endl;
		}
	}
	
	return 0;
}
