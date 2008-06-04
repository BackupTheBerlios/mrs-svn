# Perl module voor het parsen van pdb
#
# $Id: dssp.pm 18 2006-03-01 15:31:09Z hekkel $
#
# Copyright (c) 2005
#      CMBI, Radboud University Nijmegen. All rights reserved.
#
# This code is derived from software contributed by Maarten L. Hekkelman
# and Hekkelman Programmatuur b.v.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#        This product includes software developed by the Radboud University
#        Nijmegen and its contributors.
# 4. Neither the name of Radboud University nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE RADBOUD UNIVERSITY AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

package MRS::Parser::dssp;

our @ISA = "MRS::Parser";

sub new
{
	my $invocant = shift;
	my $self = new MRS::Parser(
		name		=> 'DSSP',
		url			=> 'http://www.cmbi.kun.nl/gv/dssp/',
		section		=> 'structure',
		compression	=> 'bzip',
		meta		=> [ 'title' ],
		raw_files	=> qr/\.dssp$/,
		@_
	);
	return bless $self, "MRS::Parser::dssp";
}

sub Parse
{
	my $self = shift;
	
	my ($doc, $m, $state, $title, $compound);
	
	# we assume we have only one record per parse call.

	$state = 0;
	$compound = "";
	
	while (my $line = $self->GetLine())
	{
		$doc .= $line;
		chomp($line);
			
		if ($state == 0 and $line =~ /^HEADER(.+?)\d\d-[A-Z]{3}-\d\d\s+(.{4})/o)
		{
			$title = $1;

			my $id = $2;
			
			$self->IndexValue('id', $id);
			
			$state = 1;
			$title =~ s/\s+$//;
		}
		elsif ($state == 1)
		{
			if ($line =~ /^## ALIGNMENTS/)
			{
				$state = 2;
			}
			elsif ($line =~ /^COMPND   (MOLECULE: )?( |\d )(.+)/mo)
			{
				my $cmp = $3;
				$cmp =~ s/\s+;?\s*$//;
				
				$title .= '; ' . lc($cmp);
			}
			elsif ($line =~ /^AUTHOR\s+(.+)/o)
			{
				$line = $1;
				$line =~ s/(\w)\.(?=\w)/$1. /og;
				$self->IndexText('ref', $line);
			}
			else
			{
				$self->IndexText('text', $line);
			}
		}
	}
	
	$self->StoreMetaData('title', $title);

	$self->Store($doc);
	$self->FlushDocument;
}

1;
