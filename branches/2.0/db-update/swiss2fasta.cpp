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
 
//	This is an extremely simple program to quickly convert a swissprot-like
//	file into a FastA file.

#include <iostream>
#include <string>

using namespace std;

int main(int argc, const char* argv[])
{
	string db = ">";

	if (argc == 3 and strcmp(argv[1], "-d") == 0)
	{
		db += argv[2];
		db += ':';
	}
	else if (argc != 1)
	{
		cerr << "Usage swiss2fasta [-d db_name]" << endl;
		exit(1);
	}

	string line, id;
	getline(cin, line);
	
	while (not cin.eof())
	{
		if (line[0] == 'I' and line[1] == 'D')
		{
			id = db;
			for (string::iterator c = line.begin() + 5; c != line.end(); ++c)
			{
				if (isspace(*c) or *c == ';')
					break;
				
				id += *c;
			}
			
			id += ' ';
		}
		else if (line[0] == 'D' and line[1] == 'E')
		{
			while (line[line.length() - 1] == '\n')
			{
				line.erase(line.length() - 1, 1);
			}

			id += line.substr(5, string::npos);
			id += ' ';
		}
		else if (line[0] == 'S' and line[1] == 'Q')
		{
			cout << id << endl;
			
			getline(cin, line);
			
			string seq;
			
			while (not cin.eof() and (line[0] != '/' or line[1] != '/'))
			{
				for (string::iterator c = line.begin() + 5; c != line.end(); ++c)
				{
					if (isalpha(*c))
					{
						if (seq.length() >= 72)
						{
							cout << seq << endl;
							seq.clear();
						}

						seq += *c;
					}
				}
				getline(cin, line);
			}
			
			cout << seq << endl;
		}
		
		getline(cin, line);
	}
}
